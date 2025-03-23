/******************************************************************************
* Copyright © 2024-2025 mapaware.top, 350137278@qq.com. All rights reserved.  *
*                                                                             *
* THIS SOFTWARE WAS COMPLETED WITH THE HELP OF “DeepSeek-R1”                  *
*                                                                             *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR  *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF  MERCHANTABILITY,   *
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL    *
* [mapaware.top] BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER *
* IN AN ACTION OF CONTRACT,  TORT OR OTHERWISE,  ARISING FROM, OUT OF OR IN   *
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.  *
******************************************************************************/
/*
** @file membuff.c
** @brief 连续内存池实现 - 高效管理固定大小内存块的分配与释放
** @author LiangZhang <350137278@qq.com>
** @version 0.1.0
** @since 2025-03-14 02:54:00
** @date 2025-03-24 04:23:00
*/
#include "membuff.h"
#include "uatomic.h"  // 原子操作库
#include "memalign.h" // 内存对齐分配和释放

/* 平台相关头文件包含 */
#if defined(__x86_64__) || defined(__aarch64__) || defined(_M_X64) || defined(_M_AMD64) || defined(__LP64__)
#  include "ffs64.h"
#else
#  include "ffs32.h"
#endif

// 单个内存块大小限制 4096 B
#define MEMBUFF_BSIZE_MAX       4096U

// Flag 最大数量限制：(相当于最多 4096 x 64 = 262144 内存块)
#define MEMBUFF_FLAGS_MAX       4096U


// 每个标志管理的内存块数
#if defined(FFS64_BITS)
  typedef FFS64_t membuff_flag_t;

  # define MBUF_FLAG_BITS        FFS64_BITS
  # define MBUF_FLAG_MAX         FFS64_MAX

  # define MBUF_StaticAssert     FFS64_StaticAssert
  # define MBUF_Assert           FFS64_Assert

  # define MBUF_LeftMaskFlag     FFS64_LeftMask
  # define MBUF_FindSetbits      FFS64_flags_find_setbits
  # define MBUF_MaskBits         FFS64_flags_mask_bits

#elif defined(FFS32_BITS)
  typedef FFS32_t membuff_flag_t;

  # define MBUF_FLAG_BITS        FFS32_BITS
  # define MBUF_FLAG_MAX         FFS32_MAX

  # define MBUF_StaticAssert     FFS32_StaticAssert
  # define MBUF_Assert           FFS32_Assert

  # define MBUF_next_setbit      FFS32_next_setbit
  # define MBUF_next_unsetbit    FFS32_next_unsetbit

  # define MBUF_LeftMaskFlag     FFS32_LeftMask
  # define MBUF_FindSetbits      FFS32_flags_find_setbits
  # define MBUF_MaskBits         FFS32_flags_mask_bits

#else
  # error "FFS32_BITS or FFS64_BITS not included"
#endif


// 自旋锁最大重试次数（指数退避）: 10 是比较好的
#define MBUF_SPINS_MAX       10

// 确保每个线程的内存块按缓存行（通常64字节）对齐，避免伪共享（False Sharing）
// 单个内存块最小限制（对齐大小）
#define MBUF_ALIGN_SIZE      128U

// 单个内存块大小限制 4096 B
#define MBUF_BSIZE_MAX       MEMBUFF_BSIZE_MAX

// Flag 最大数量限制：(相当于最多 4096 x 64 = 262144 Buffs)
#define MBUF_FLAGS_MAX       MEMBUFF_FLAGS_MAX

// 内存池总大小限制 1 GB (可能受实际可分配内存限制)
#define MBUF_POOL_SIZE_MAX   (MBUF_FLAGS_MAX * MBUF_BSIZE_MAX * 64)

// 对齐字节, 0 对齐为 M
#define MBUF_AlignUpSize(size, M)   ((size)==0? ((membuff_flag_t)(M)): (((membuff_flag_t)(size) + (membuff_flag_t)(M) - 1) & ~((membuff_flag_t)(M)-1)))


// 每个内存块大小固定：BuffSize
// 每个内存块由一个标记位记录是1否0空闲
// 可供分配的内存 = BuffSize x N
//   N = 1, 2, 3, ... 32
typedef struct membuff_pool_t
{
    uint32_t BuffSize;         // 每个可分配的内存块的最小尺寸
    uint32_t FlagsCount;       // Flag 的数量

    uatomic_bool spinlock;      // 自旋锁：多线程安全访问

    uatomic_int unused_bits;    // 未使用的（位=1）内存块

    size_t _PoolSize;
    char* _Buffers;
    membuff_flag_t  _Flags[];
} membuff_pool_t;

// 内存头信息16字节（位于每个分配块的首部）
typedef struct
{
    membuff_pool_t* ownerPool; // 所属的内存池
    uint32_t flagOffset;       // 所属的flag结构在池中的索引
    uint16_t bitOffset;        // 在flag中的起始位索引（0-based）
    uint16_t bitCount;         // 占用的连续内存块数（1bit对应一个内存块）
    char _Buffer[];            // 实际内存起始地址
} membuff_t;

// 确保标志位数正确
MBUF_StaticAssert(MBUF_FLAG_BITS == sizeof(membuff_flag_t) * 8, MBUF_FLAG_BITS_must_be_flag_bits);

// 对齐是头大小的整数倍
MBUF_StaticAssert(MBUF_ALIGN_SIZE % sizeof(membuff_t) == 0, MBUF_ALIGN_SIZE_must_be_align_membuff);


membuff_pool membuff_pool_create(size_t buffSizeBytes, size_t buffsCount)
{
    MBUF_Assert(buffSizeBytes <= MBUF_BSIZE_MAX);
    MBUF_Assert(buffsCount <= MBUF_FLAGS_MAX);

    // 内存块大小是 MBUF_ALIGN_SIZE 字节的倍数
    const uint32_t buffSize = (uint32_t)MBUF_AlignUpSize(buffSizeBytes, MBUF_ALIGN_SIZE);
    if (buffSize > MBUF_BSIZE_MAX) {
        fprintf(stderr, "buffSizeBytes(=%zu) is more than MBUF_BSIZE_MAX(=%zu).\n", (size_t)buffSize, (size_t)MBUF_BSIZE_MAX);
        return NULL;
    }

    // 内存块数量是 MBUF_FLAG_BITS 的整数倍
    const uint32_t numBits = (uint32_t)MBUF_AlignUpSize(buffsCount, MBUF_FLAG_BITS);

    // numFlags: 用多少个 flag 整数标记内存的状态位：1 空闲, 0 占用
    const uint32_t numFlags = numBits / MBUF_FLAG_BITS;
    if (numFlags == 0 || numFlags > MBUF_FLAGS_MAX) {
        fprintf(stderr, "number of flags(=%d) is more than MBUF_FLAGS_MAX(=%d).\n", numFlags, MBUF_FLAGS_MAX);
        return NULL;
    }

    const size_t poolSizeMax = (size_t) buffSize * numBits;
    if (poolSizeMax > MBUF_POOL_SIZE_MAX) {
        fprintf(stderr, "pool size(=%zu) is more than MBUF_POOL_SIZE_MAX(=%zu).\n", poolSizeMax, (size_t)MBUF_POOL_SIZE_MAX);
        return NULL;
    }

    // numFlags: 用多少个 flag 整数标记内存的状态位：1 空闲, 0 占用
    size_t size = sizeof(membuff_pool_t) + sizeof(membuff_flag_t) * numFlags + poolSizeMax;
    membuff_pool_t* pool = (membuff_pool_t*)malloc(size);
    if (!pool) {
        return NULL;
    }
    memset(pool, 0, size);

    pool->BuffSize = buffSize;        // 修改只读字段（仅初始化时）
    pool->FlagsCount = numFlags;      // 修改只读字段（仅初始化时）

    pool->_Buffers = (char*)&pool->_Flags[numFlags];
    pool->_PoolSize = poolSizeMax;

    pool->unused_bits = (int)(MBUF_FLAG_BITS * pool->FlagsCount);

    // 设置全部块为空闲: flag = 1
    for (uint32_t i = 0; i < pool->FlagsCount; ++i) {
        pool->_Flags[i] = MBUF_FLAG_MAX;
    }

    return pool;
}


void membuff_pool_destroy(void* pPool)
{
    if (pPool) {
        free(pPool);
    }
}


void* membuff_alloc(membuff_pool pool, size_t sizeBytes)
{
    if (!pool) {
        // 无池分配
        return malloc(sizeBytes);
    }

    // 需要的内存字节
    size_t allocSize = MBUF_AlignUpSize(sizeBytes + sizeof(membuff_t), pool->BuffSize);

    // 需要的内存块数
    int bitCount = (int)(allocSize / pool->BuffSize);

    // 计算最大可分配的容量
    if (bitCount == 0 || bitCount > (int)(MBUF_FLAG_BITS * pool->FlagsCount)) {
        return NULL;  // 超过最大块
    }

    if (bitCount > uatomic_int_get(&pool->unused_bits)) {
        return NULL;  // 空闲块不满足, 直接尝试下一个
    }

    bool_spinlock_grab(&pool->spinlock, MBUF_SPINS_MAX);

    membuff_flag_t* pFlag = pool->_Flags;
    int startBit = MBUF_FindSetbits(&pFlag, pool->_Flags + pool->FlagsCount, bitCount);
    if (!startBit) {
        bool_spinlock_free(&pool->spinlock);
        return NULL;
    }

    // startBit 位偏移从 0 开始：0-based
    --startBit;

    int flagOffset = (int)(pFlag - pool->_Flags);
    int bitOffset = flagOffset * MBUF_FLAG_BITS + startBit;

    // 计算块偏移以得到块的位置
    membuff_t* pBuff = (membuff_t*)&pool->_Buffers[bitOffset * pool->BuffSize];

    // 设置块头
    pBuff->flagOffset = flagOffset;
    pBuff->bitOffset = (uint16_t)startBit;  // 块位偏移从 0 开始
    pBuff->bitCount = (uint16_t)bitCount;   // 实际跨越的块数
    pBuff->ownerPool = pool;                // Ownership

    // 清除块空闲位=0: 标记已经使用
    MBUF_MaskBits(pFlag, startBit, bitCount, 0);

    uatomic_int_sub_n(&pool->unused_bits, bitCount);

    bool_spinlock_free(&pool->spinlock);

    // 返回实际内存块的起始地址
    return (void*)pBuff->_Buffer;
}


void* membuff_calloc(membuff_pool pool, size_t elementsCount, size_t elementSizeBytes)
{
    if (!pool) {
        // 无池分配
        return calloc(elementsCount, elementSizeBytes);
    }
    void* pBuffer = membuff_alloc(pool, elementSizeBytes * elementsCount);
    if (pBuffer) {
        memset(pBuffer, 0, elementSizeBytes * elementsCount);
    }
    return pBuffer;
}

void* membuff_free(membuff_pool ownerPool, void* pBuffer)
{
    if (!pBuffer) {
        return NULL;
    }

    if (!ownerPool) {
        // 无池释放
        free(pBuffer);
        return NULL;
    }

    membuff_t* pBuff = (membuff_t*)((char*)pBuffer - offsetof(membuff_t, _Buffer));
    membuff_pool pool = pBuff->ownerPool;
    if (pool == ownerPool) {
        size_t bBuffSize = pool->BuffSize * pBuff->bitCount;

        MBUF_Assert((char*)pBuff >= pool->_Buffers && (char*)pBuff + bBuffSize <= (char*)pool->_Buffers + pool->_PoolSize);

        if ((char*)pBuff >= pool->_Buffers && (char*)pBuff + bBuffSize <= (char*)pool->_Buffers + pool->_PoolSize) {

            bool_spinlock_grab(&pool->spinlock, MBUF_SPINS_MAX);

            // 生成掩码：将 bitOffset 开始的 bitCount 个位设为 1，其余位不变
            MBUF_MaskBits(pool->_Flags + pBuff->flagOffset, pBuff->bitOffset, pBuff->bitCount, 1);

            uatomic_int_add_n(&pool->unused_bits, pBuff->bitCount);

            bool_spinlock_free(&pool->spinlock);

            // 成功释放，返回 0
            return NULL;
        }
    }
    // 释放失败，返回原内存指针
    return pBuffer;
}


size_t membuff_pool_stat(membuff_pool pool, membuff_stat_t* pStat)
{
    if (pStat) {
        pStat->buffSizeBytes = pool->BuffSize;
        pStat->buffsMaxCount = pool->FlagsCount * MBUF_FLAG_BITS;
        getnowtimeofday(&pStat->timestamp);
    }
    return (size_t) uatomic_int_get(&pool->unused_bits);
}
