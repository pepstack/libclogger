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
** @file membulk.h
** @brief 连续内存池实现 - 高效管理固定大小内存块的分配与释放
** @author LiangZhang <350137278@qq.com>
** @version 0.1.0
** @since 2025-03-14 02:54:00
** @date 2025-03-20 14:23:00
*/
#include "membulk.h"

#include "timeut.h"   // 时间工具，用于退避等待
#include "uatomic.h"  // 原子操作库

#include <stddef.h> //  offsetof

/* 平台相关头文件包含 */
#if defined(__x86_64__) || defined(__aarch64__) || defined(_M_X64) || defined(_M_AMD64) || defined(__LP64__)
#  include "ffs64.h"
#else
#  include "ffs32.h"
#endif


// 每个标志管理的内存块数
#if defined(FFS64_BITS)
  typedef FFS64_t membulk_flag_t;

  # define MBK_StaticAssert FFS64_StaticAssert
  # define MBK_Assert FFS64_Assert

  # define MBK_next_setbit      FFS64_next_setbit
  # define MBK_next_unsetbit    FFS64_next_unsetbit

  # define MBK_FLAG_BITS        FFS64_BITS
  # define MBK_FLAG_MAX         FFS64_MAX
#elif defined(FFS32_BITS)
  typedef FFS32_t membulk_flag_t;

  # define MBK_StaticAssert FFS32_StaticAssert
  # define MBK_Assert FFS32_Assert

  # define MBK_next_setbit      FFS32_next_setbit
  # define MBK_next_unsetbit    FFS32_next_unsetbit

  # define MBK_FLAG_BITS        FFS32_BITS
  # define MBK_FLAG_MAX         FFS32_MAX
#else
  error: FFS32_BITS or FFS64_BITS not included;
#endif


// 自旋锁最大重试次数（指数退避）: 10 是比较好的
#define MBK_SPINS_MAX       10

// 内存块字节尺寸对齐大小 (b4Bits=128)
#define MBK_ALIGN_SIZE      128U

// 单个内存块大小限制 1 MB
#define MBK_BSIZE_MAX       1048576U

// Flag 最大数量限制：256 (相当于最多 256 x 64 = 16384 Bulks)
#define MBK_FLAGS_MAX       256U

// 内存池总大小限制 1280 MB (可能受实际可分配内存限制)
#define MBK_POOL_SIZE_MAX   (1280U * MBK_BSIZE_MAX)

// 掩码表数值未考虑字节序问题???
#if (MBK_FLAG_BITS == 64)
  // 预定义掩码表：bitCount=0~64 对应的掩码: 连续 bitCount 个1（低 bitCount 位为1）
  static const membulk_flag_t MASK_flag_table[MBK_FLAG_BITS + 1] = { 0x0000000000000000,
      0x0000000000000001, 0x0000000000000003, 0x0000000000000007, 0x000000000000000F,
      0x000000000000001F, 0x000000000000003F, 0x000000000000007F, 0x00000000000000FF,
      0x00000000000001FF, 0x00000000000003FF, 0x00000000000007FF, 0x0000000000000FFF,
      0x0000000000001FFF, 0x0000000000003FFF, 0x0000000000007FFF, 0x000000000000FFFF,
      0x000000000001FFFF, 0x000000000003FFFF, 0x000000000007FFFF, 0x00000000000FFFFF,
      0x00000000001FFFFF, 0x00000000003FFFFF, 0x00000000007FFFFF, 0x0000000000FFFFFF,
      0x0000000001FFFFFF, 0x0000000003FFFFFF, 0x0000000007FFFFFF, 0x000000000FFFFFFF,
      0x000000001FFFFFFF, 0x000000003FFFFFFF, 0x000000007FFFFFFF, 0x00000000FFFFFFFF,
      0x00000001FFFFFFFF, 0x00000003FFFFFFFF, 0x00000007FFFFFFFF, 0x0000000FFFFFFFFF,
      0x0000001FFFFFFFFF, 0x0000003FFFFFFFFF, 0x0000007FFFFFFFFF, 0x000000FFFFFFFFFF,
      0x000001FFFFFFFFFF, 0x000003FFFFFFFFFF, 0x000007FFFFFFFFFF, 0x00000FFFFFFFFFFF,
      0x00001FFFFFFFFFFF, 0x00003FFFFFFFFFFF, 0x00007FFFFFFFFFFF, 0x0000FFFFFFFFFFFF,
      0x0001FFFFFFFFFFFF, 0x0003FFFFFFFFFFFF, 0x0007FFFFFFFFFFFF, 0x000FFFFFFFFFFFFF,
      0x001FFFFFFFFFFFFF, 0x003FFFFFFFFFFFFF, 0x007FFFFFFFFFFFFF, 0x00FFFFFFFFFFFFFF,
      0x01FFFFFFFFFFFFFF, 0x03FFFFFFFFFFFFFF, 0x07FFFFFFFFFFFFFF, 0x0FFFFFFFFFFFFFFF,
      0x1FFFFFFFFFFFFFFF, 0x3FFFFFFFFFFFFFFF, 0x7FFFFFFFFFFFFFFF, 0xFFFFFFFFFFFFFFFF
  };
#else
  // 预定义掩码表：bitCount=0~32 对应的掩码: 连续 bitCount 个1
  static const membulk_flag_t MASK_flag_table[MBK_FLAG_BITS + 1] = { 0x00000000,
      0x00000001, 0x00000003, 0x00000007, 0x0000000F,
      0x0000001F, 0x0000003F, 0x0000007F, 0x000000FF,
      0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF,
      0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF,
      0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF,
      0x001FFFFF, 0x003FFFFF, 0x007FFFFF, 0x00FFFFFF,
      0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF,
      0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF
  };
#endif


/* 跨平台packed定义 */
#if defined(__GNUC__) || defined(__clang__)
#  define MBK_PACKED __attribute__((packed, aligned(1)))
#elif defined(_MSC_VER)
#  define MBK_PACKED __pragma(pack(push, 1))
#  define MBK_END_PACKED __pragma(pack(pop))
#else
#  error "Compiler not supported"
#endif


// 每个内存块大小固定：BulkSize
// 每个内存块由一个标记位记录是1否0空闲
// 可供分配的内存 = BulkSize x N
//   N = 1, 2, 3, ... 32
typedef struct membulk_pool_t
{
    uint32_t BulkSize;         // 每个可分配的内存块的最小尺寸
    uint32_t FlagsCount;       // Flag 的数量

    uatomic_int spinlock;      // 自旋锁：多线程安全访问
    volatile int unused_bits;  // 未使用的（位）内存块

    unsigned char* _Bulks;
    membulk_flag_t  _Flags[];
} membulk_pool_t;


// 内存头信息16字节（位于每个分配块的首部）
typedef struct
{
    membulk_pool_t* ownerPool; // 所属的内存池
    uint32_t flagOffset;       // 所属的flag结构在池中的索引
    uint16_t bitOffset;        // 在flag中的起始位索引（0-based）
    uint16_t bitCount;         // 占用的连续内存块数（1bit对应一个内存块）
    char _Memory[];            // 实际内存起始地址
} membulk_t;

// 确保标志位数正确
MBK_StaticAssert(MBK_FLAG_BITS == sizeof(membulk_flag_t) * 8, MBK_FLAG_BITS_must_be_flag_bits);

// 对齐是头大小的整数倍
MBK_StaticAssert(MBK_ALIGN_SIZE % sizeof(membulk_t) == 0, MBK_ALIGN_SIZE_must_be_align_membulk);


/**
 * @brief 对齐内存大小
 * @param size 原始大小
 * @param alignSize 对齐粒度
 * @return 对齐后的内存大小
 */
static inline size_t _mbk_align_size(size_t size, size_t alignSize)
{
    return (size ? ((size + alignSize - 1) / alignSize) : 1) * alignSize;
}


/**
 * @brief 获取自旋锁（强等待，指数退避后让出CPU）
 * @param spinlockAddr 自旋锁地址
 */
static void _mbk_spinlock_grab(uatomic_int* spinlockAddr, int spinsMax)
{
    // 一直等待
    int spins = 0;

    while (uatomic_int_comp_exch(spinlockAddr, 0, 1)) {
        ++spins;
        if (spins >= spinsMax) {
            sleep_usec((1 << spins));
            spins = 0;
        }
    }
}


static void inline _mbk_spinlock_free(uatomic_int* spinlockAddr)
{
    MBK_Assert(uatomic_int_get(spinlockAddr) == 1);
    uatomic_int_set(spinlockAddr, 0);
}


// flags=[flag1|flag2|flag3] => [0b1111000 | 0b11111111 | 0b00000111]，连续块数=4+8+3=15
// 1 flag = 64 Bits = 64 个内存块
static int _mkt_find_bulk_setbits(membulk_flag_t** ppFlagStart, const membulk_flag_t* pFlagEndStop, const int bulkBitsCount)
{
    membulk_flag_t* pFirstFlag = NULL;
    int firstBitOffset = 0;

    int startBit = 1;
    int remaining = bulkBitsCount;

    membulk_flag_t* pFlag = *ppFlagStart;
    while (pFlag < pFlagEndStop && remaining > 0) {
        MBK_Assert(startBit > 0);  // startBit: 1-based

        startBit = MBK_next_setbit(*pFlag, startBit);
        if (!startBit) { // 无置位
            remaining = bulkBitsCount;  // 重置
            pFirstFlag = NULL;
            ++pFlag;
            startBit = 1;
            continue;
        }

        if (pFirstFlag) {
            // 已经设置起始，startBit 必须从 1 开始
            if (startBit != 1) {
                // 出现清零位，重置（pFlag 不能移动）
                remaining = bulkBitsCount;
                pFirstFlag = pFlag;
                firstBitOffset = startBit;
            }
        }
        else { // 仅在此处设置起始
            remaining = bulkBitsCount;
            pFirstFlag = pFlag;
            firstBitOffset = startBit;
        }

        MBK_Assert(pFirstFlag && startBit);

        // endBit=0 表示从第 startBit 位（0-based）开始到结束没有清0位。
        int endBit = MBK_next_unsetbit(*pFlag, startBit + 1);
        int available = endBit ? endBit - startBit : MBK_FLAG_BITS - startBit + 1;

        if (remaining <= available) {
            // 发现可用的连续位，成功仅在此处返回
            *ppFlagStart = pFirstFlag;
            return firstBitOffset;
        }

        // 不满足连续置位数: remaining > available
        if (endBit) { // 出现清零位，重置（pFlag 不能移动）
            remaining = bulkBitsCount;
            pFirstFlag = NULL;
            startBit = endBit;  // 防止无限循环
        }
        else {
            MBK_Assert(endBit == 0);  // 不存在清零位
            remaining -= available;
            startBit = 1;
            ++pFlag; // 下一个 Flag
        }
    }

    // 未发现可用的连续位(buckBitsCount)
    return 0;
}


membulk_pool membulk_pool_create(uint32_t bulkSizeBytes, uint32_t bulksCapacity)
{
    MBK_Assert(bulkSizeBytes <= MBK_BSIZE_MAX);
    MBK_Assert((bulkSizeBytes * bulksCapacity) <= MBK_POOL_SIZE_MAX);

    // 内存块大小是 MBK_ALIGN_SIZE 字节的倍数
    const uint32_t bulkSize = (uint32_t)_mbk_align_size(bulkSizeBytes, MBK_ALIGN_SIZE);
    if (bulkSize > MBK_BSIZE_MAX) {
        fprintf(stderr, "bulkSizeBytes(=%zu) is more than MBK_BSIZE_MAX(=%zu).\n", (size_t)bulkSize, (size_t)MBK_BSIZE_MAX);
        return NULL;
    }

    // 内存块数量是 MBK_FLAG_BITS 的整数倍
    const uint32_t numBits = (uint32_t)_mbk_align_size(bulksCapacity, MBK_FLAG_BITS);

    // numFlags: 用多少个 flag 整数标记内存的状态位：1 空闲, 0 占用
    const uint32_t numFlags = numBits / MBK_FLAG_BITS;
    if (numFlags > MBK_FLAGS_MAX) {
        fprintf(stderr, "number of flags(=%d) is more than MBK_FLAGS_MAX(=%d).\n", numFlags, MBK_FLAGS_MAX);
        return NULL;
    }

    const size_t poolSizeMax = (size_t) bulkSize * numBits;
    if (poolSizeMax > MBK_POOL_SIZE_MAX) {
        fprintf(stderr, "pool size(=%zu) is more than MBK_POOL_SIZE_MAX(=%zu).\n", poolSizeMax, (size_t)MBK_POOL_SIZE_MAX);
        return NULL;
    }

    // numFlags: 用多少个 flag 整数标记内存的状态位：1 空闲, 0 占用
    size_t size = sizeof(membulk_pool_t) + sizeof(membulk_flag_t) * numFlags + poolSizeMax;
    membulk_pool_t* pool = (membulk_pool_t*)malloc(size);
    if (!pool) {
        return NULL;
    }
    memset(pool, 0, size);

    pool->BulkSize = bulkSize;        // 修改只读字段（仅初始化时）
    pool->FlagsCount = numFlags;      // 修改只读字段（仅初始化时）

    pool->_Bulks = (unsigned char*)&pool->_Flags[numFlags];
    pool->unused_bits = MBK_FLAG_BITS * pool->FlagsCount;

    // 设置全部块为空闲: flag = 1
    for (uint32_t i = 0; i < pool->FlagsCount; ++i) {
        pool->_Flags[i] = MBK_FLAG_MAX;
    }

    return pool;
}


void membulk_pool_destroy(membulk_pool pool)
{
    if (pool) {
        free(pool);
    }
}


void* membulk_alloc(membulk_pool pool, uint32_t sizeBytes)
{
    // 需要的内存字节
    size_t allocSize = _mbk_align_size(sizeBytes + sizeof(membulk_t), pool->BulkSize);

    // 需要的内存块数
    int bitCount = (int)(allocSize / pool->BulkSize);

    // 计算最大可分配的容量
    if (bitCount == 0 || bitCount > (int)(MBK_FLAG_BITS * pool->FlagsCount)) {
        return NULL;  // 超过最大块
    }

    if (bitCount > pool->unused_bits) {
        return NULL;  // 空闲块不满足, 直接尝试下一个
    }

    _mbk_spinlock_grab(&pool->spinlock, MBK_SPINS_MAX);

    membulk_flag_t* pFlag = pool->_Flags;
    int startBit = _mkt_find_bulk_setbits(&pFlag, pool->_Flags + pool->FlagsCount, bitCount);
    if (!startBit) {
        _mbk_spinlock_free(&pool->spinlock);
        return NULL;
    }

    // startBit 位偏移从 0 开始：0-based
    --startBit;

    int flagOffset = (int)(pFlag - pool->_Flags);
    int bitOffset = flagOffset * MBK_FLAG_BITS + startBit;

    // 计算块偏移以得到块的位置
    membulk_t* pBulk = (membulk_t*)&pool->_Bulks[bitOffset * pool->BulkSize];

    // 设置块头
    pBulk->flagOffset = flagOffset;
    pBulk->bitOffset = (uint16_t)startBit;  // 块位偏移从 0 开始
    pBulk->bitCount = (uint16_t)bitCount;   // 实际跨越的块数
    pBulk->ownerPool = pool;                // Ownership

    pool->unused_bits -= bitCount;

    // 清除块空闲位: 标记已经使用
    while (bitCount > 0) {
        // 需要清零的位数
        bitOffset = MBK_FLAG_BITS - startBit;
        if (bitOffset > bitCount) {
            bitOffset = bitCount;
        }
        
        // 生成掩码 (需要清除的位为0)
        membulk_flag_t mask = ~(MASK_flag_table[bitOffset] << startBit);
        *pFlag &= mask; // 清除对应的位

        bitCount -= bitOffset;
        if (bitCount == 0) {
            break;  // 成功设置
        }

        startBit = 0;
        ++pFlag;
    }

    _mbk_spinlock_free(&pool->spinlock);

    // 返回实际内存块的起始地址
    return (void*)pBulk->_Memory;
}


void* membulk_free(void* pMemory)
{
    MBK_Assert(pMemory);
    if (!pMemory) {
        return NULL;
    }

    membulk_t* pBulk = (membulk_t*)((char*)pMemory - offsetof(membulk_t, _Memory));
    membulk_pool pool = pBulk->ownerPool;
    MBK_Assert(pool);
    if (pool) {
        const size_t poolSizeMax = (size_t) pool->BulkSize * pool->FlagsCount * MBK_FLAG_BITS;

        MBK_Assert((char*)pBulk >= (char*)pool->_Bulks && (char*)pBulk < (char*)pool->_Bulks + poolSizeMax);

        if ((char*)pBulk >= (char*)pool->_Bulks && (char*)pBulk < (char*)pool->_Bulks + poolSizeMax) {
            _mbk_spinlock_grab(&pool->spinlock, MBK_SPINS_MAX);

            membulk_flag_t* pFlagOffset = pool->_Flags + pBulk->flagOffset;
            int startBit = pBulk->bitOffset;
            int bitCount = pBulk->bitCount;
            while (bitCount > 0) {
                // 需要置位位数
                int bitOffset = bitCount > MBK_FLAG_BITS - startBit ? MBK_FLAG_BITS - startBit : bitCount;

                // 生成掩码：将 bitOffset 开始的 bitCount 个位设为 1，其余位不变
                membulk_flag_t mask = (membulk_flag_t)(MASK_flag_table[bitOffset] << startBit);
                *pFlagOffset |= mask; // 置位对应的位

                startBit = 0;
                bitCount -= bitOffset;

                ++pFlagOffset;
            }
            pool->unused_bits += (int)pBulk->bitCount;

            // success
            _mbk_spinlock_free(&pool->spinlock);
            return NULL;
        }
    }
    return pMemory;
}


int membulk_pool_stats(membulk_pool pool, membulk_stats_t* stats)
{
    if (stats) {
        stats->sizeBytes = pool->BulkSize;
        stats->capacity = pool->FlagsCount * MBK_FLAG_BITS;
    }

    return pool->unused_bits;
}
