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
** @file membuff.h
** @brief 连续内存池实现 - 高效管理固定大小内存块的分配与释放
** @author LiangZhang <350137278@qq.com>
** @version 0.1.0
** @since 2025-03-14 02:54:00
** @date 2025-03-21 14:23:00
*/
#include "membuff.h"
#include "uatomic.h"  // 原子操作库

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
  # define MBUF_FindSetbits      FFS64_flags_setbits
#elif defined(FFS32_BITS)
  typedef FFS32_t membuff_flag_t;

  # define MBUF_FLAG_BITS        FFS32_BITS
  # define MBUF_FLAG_MAX         FFS32_MAX

  # define MBUF_StaticAssert     FFS32_StaticAssert
  # define MBUF_Assert           FFS32_Assert

  # define MBUF_next_setbit      FFS32_next_setbit
  # define MBUF_next_unsetbit    FFS32_next_unsetbit

  # define MBUF_LeftMaskFlag     FFS32_LeftMask
  # define MBUF_FindSetbits      FFS32_flags_setbits
#else
  # error "FFS32_BITS or FFS64_BITS not included"
#endif


// 自旋锁最大重试次数（指数退避）: 10 是比较好的
#define MBUF_SPINS_MAX       10

// 确保每个线程的内存块按缓存行（通常64字节）对齐，避免伪共享（False Sharing）
// 单个内存块最小限制（对齐大小）
#define MBUF_ALIGN_SIZE       128U

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

    uatomic_int spinlock;      // 自旋锁：多线程安全访问
    volatile int unused_bits;  // 未使用的（位）内存块

    unsigned char* _Buffers;
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


membuff_pool membuff_pool_create(uint32_t buffSizeBytes, uint32_t buffsCount)
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

    pool->_Buffers = (unsigned char*)&pool->_Flags[numFlags];
    pool->unused_bits = MBUF_FLAG_BITS * pool->FlagsCount;

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

    if (bitCount > pool->unused_bits) {
        return NULL;  // 空闲块不满足, 直接尝试下一个
    }

    uatomic_spinlock_grab(&pool->spinlock, MBUF_SPINS_MAX);

    membuff_flag_t* pFlag = pool->_Flags;
    int startBit = MBUF_FindSetbits(&pFlag, pool->_Flags + pool->FlagsCount, bitCount);
    if (!startBit) {
        uatomic_spinlock_free(&pool->spinlock);
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

    pool->unused_bits -= bitCount;

    // 清除块空闲位: 标记已经使用
    while (bitCount > 0) {
        // 需要清零的位数
        bitOffset = MBUF_FLAG_BITS - startBit;
        if (bitOffset > bitCount) {
            bitOffset = bitCount;
        }
        
        // 生成掩码 (需要清除的位为0)
        membuff_flag_t mask = ~MBUF_LeftMaskFlag(bitOffset, startBit);
        *pFlag &= mask; // 清除对应的位

        bitCount -= bitOffset;
        if (bitCount == 0) {
            break;  // 成功设置
        }

        startBit = 0;
        ++pFlag;
    }

    uatomic_spinlock_free(&pool->spinlock);

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
        const size_t poolSizeMax = (size_t) pool->BuffSize * pool->FlagsCount * MBUF_FLAG_BITS;

        MBUF_Assert((char*)pBuff >= (char*)pool->_Buffers && (char*)pBuff < (char*)pool->_Buffers + poolSizeMax);

        if ((char*)pBuff >= (char*)pool->_Buffers && (char*)pBuff < (char*)pool->_Buffers + poolSizeMax) {
            uatomic_spinlock_grab(&pool->spinlock, MBUF_SPINS_MAX);

            membuff_flag_t* pFlagOffset = pool->_Flags + pBuff->flagOffset;
            int startBit = pBuff->bitOffset;
            int bitCount = pBuff->bitCount;
            while (bitCount > 0) {
                // 需要置位位数
                int bitOffset = bitCount > MBUF_FLAG_BITS - startBit ? MBUF_FLAG_BITS - startBit : bitCount;

                // 生成掩码：将 bitOffset 开始的 bitCount 个位设为 1，其余位不变
                membuff_flag_t mask = MBUF_LeftMaskFlag(bitOffset, startBit);
                *pFlagOffset |= mask; // 置位对应的位

                startBit = 0;
                bitCount -= bitOffset;

                ++pFlagOffset;
            }
            pool->unused_bits += (int)pBuff->bitCount;

            uatomic_spinlock_free(&pool->spinlock);

            // 成功释放，返回 0
            return NULL;
        }
    }
    // 释放失败，返回原内存指针
    return pBuffer;
}


int membuff_pool_stats(membuff_pool pool, membuff_stats_t* stats)
{
    if (stats) {
        stats->buffSizeBytes = pool->BuffSize;
        stats->buffsMaxCount = pool->FlagsCount * MBUF_FLAG_BITS;
    }
    return pool->unused_bits;
}
