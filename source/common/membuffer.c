/***********************************************************************************************
* Copyright © 2024-2025 mapaware.top, 350137278@qq.com. All rights reserved.                   *
*                                                                                              *
* This software is a proprietary product of [mapaware.top]  (Email: 350137278@qq.com).  It is  *
* protected by copyright laws and international treaties. The following restrictions apply to  *
* the use, reproduction, and distribution of this software:                                    *
*                                                                                              *
* 1. Use                                                                                       *
*   This software may only be used in accordance with the terms and conditions of the software *
*   license agreement provided by [mapaware.top]. Any unauthorized use is strictly prohibited. *
*                                                                                              *
* 2.Reproduction                                                                               *
*   No part of this software may be reproduced,stored in a retrieval system, or transmitted in *
*   any form or by any means (electronic, mechanical, photocopying, recording, or otherwise)   *
*   without the prior written permission of [mapaware.top].                                    *
*                                                                                              *
* 3.Distribution                                                                               *
*   Redistribution of this software, whether in its original form or modified, is strictly     *
*   prohibited without the express written consent of [mapaware.top].                          *
*                                                                                              *
* Infringers of this copyright will be subject to civil and criminal penalties.                *
*                                                                                              *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,EXPRESS OR IMPLIED, INCLUDING *
* BUT NOT LIMITED TO THE WARRANTIES OF  MERCHANTABILITY,  FITNESS FOR A PARTICULAR PURPOSE AND *
* NONINFRINGEMENT.  IN NO EVENT SHALL [mapaware.top] BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER *
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT,  TORT OR OTHERWISE,  ARISING FROM, OUT OF OR IN *
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.                   *
***********************************************************************************************/
/*
** @file membuffer.c
**   内存池实现，用于高效管理固定大小的内存块分配与释放。
** @author Liang Zhang <350137278@qq.com>
** @version 0.0.1
** @since 2025-03-14 02:54:00
** @date 2025-03-17 04:31:00
*/
#include "membuffer.h"

#include "timeut.h"   // 时间工具，用于退避等待
#include "uatomic.h"  // 原子操作库

#include <assert.h>  // assert()
#include <sched.h>   // sched_yield()
#include <stddef.h> //  offsetof

// 每个标志管理的内存块数
#if defined(FFS64_FLAG_BITS)
  typedef ffs64_flag_t membuffer_flag_t;

  # define MBUF_StaticAssert FFS64_StaticAssert
  # define MBUF_Assert FFS64_Assert

  # define MBUF_setbit_popcount  FFS64_setbit_popcount
  # define MBUF_first_setbit_n   FFS64_first_setbit_n
  # define MBUF_setbit_popcount  FFS64_setbit_popcount

  # define MBUF_FLAG_BITS        FFS64_FLAG_BITS
#elif defined(FFS32_FLAG_BITS)
  typedef ffs32_flag_t membuffer_flag_t;

  # define MBUF_StaticAssert FFS32_StaticAssert
  # define MBUF_Assert FFS32_Assert

  # define MBUF_setbit_popcount  FFS32_setbit_popcount
  # define MBUF_first_setbit_n   FFS32_first_setbit_n
  # define MBUF_setbit_popcount  FFS32_setbit_popcount
  # define MBUF_FLAG_BITS        FFS32_FLAG_BITS
#else
  error: FFS32_FLAG_BITS or FFS64_FLAG_BITS not included
#endif


// 自旋锁最大重试次数（指数退避）: 10 是比较好的
#define MBUF_SPINS_MAX       10

// 内存块字节尺寸对齐大小 = 128
#define MBUF_ALIGN_SIZE      (MBUF_FLAG_BITS * 2)

// 单个内存块大小限制 8 MB
#define MBUF_SIZE_MAX        (MBUF_ALIGN_SIZE * 8192 * 8)

// 内存池总大小限制 1024 MB (可能受实际可分配内存限制)
#define MBUF_POOL_SIZE_MAX   (MBUF_SIZE_MAX * MBUF_ALIGN_SIZE)


#if (MBUF_FLAG_BITS == 64)
  // 预定义掩码表：bitCount=0~64 对应的掩码: 连续 bitCount 个1（低 bitCount 位为1）
  static const membuffer_flag_t MASK_flag_table[MBUF_FLAG_BITS + 1] = { 0x0000000000000000,
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
  static const membuffer_flag_t MASK_flag_table[MBUF_FLAG_BITS + 1] = { 0x00000000,
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


// 每个内存块大小固定：BufferSizeB
// 每个内存块由一个标记位记录是1否0空闲
// 可供分配的内存 = BufferSizeB x N
//   N = 1, 2, 3, ... 32
typedef struct membuffer_pool_t
{
    uint32_t BufferSizeB;     // size in bytes of per buffer
    uint32_t FlagsCount;      // count of Flags

    uatomic_int spinlock;     // 自旋锁
    volatile int unused_bits;  // 未使用的（位）内存块

    unsigned char* _Buffers;
    membuffer_flag_t  _Flags[];
} membuffer_pool_t;


// 内存头信息（位于每个分配块的首部）
typedef struct
{
    membuffer_pool_t* ownerPool; // 所属的内存池
    uint32_t flagOffset;         // 所属的flag结构在池中的索引
    uint16_t bitOffset;          // 在flag中的起始位索引（0-based）
    uint16_t bitCount;           // 占用的连续内存块数（1bit对应一个内存块）
    char _Memory[];             // 实际内存起始地址
} membuffer_t;

// 确保标志位数正确
MBUF_StaticAssert(MBUF_FLAG_BITS == sizeof(membuffer_flag_t) * 8, MBUF_FLAG_BITS_must_be_flag_bits);

// 对齐需容纳头结构
MBUF_StaticAssert(MBUF_ALIGN_SIZE % 64 == 0, MBUF_ALIGN_SIZE_must_be_64x);

// 对齐是头大小的整数倍
MBUF_StaticAssert(MBUF_ALIGN_SIZE % sizeof(membuffer_t) == 0, MBUF_ALIGN_SIZE_must_be_align_membuffer);


/**
 * @brief 对齐内存大小
 * @param size 原始大小
 * @param alignSize 对齐粒度
 * @return 对齐后的内存大小
 */
static inline size_t __align_size(size_t size, size_t alignSize)
{
    return ((size + alignSize - 1) / alignSize) * alignSize;
}


/**
 * @brief 获取自旋锁（强等待，指数退避后让出CPU）
 * @param spinlockAddr 自旋锁地址
 */
static void __spinlock_grab(uatomic_int* spinlockAddr, int spinsMax)
{
    // 一直等待
    int spins = 0;

    while (uatomic_int_comp_exch(spinlockAddr, 0, 1)) {
        ++spins;
        sched_yield();

        if (spins >= spinsMax) {
            sleep_usec((1 << spins));
            spins = 0;
        }
    }
}


static void inline __spinlock_free(uatomic_int* spinlockAddr)
{
    MBUF_Assert(uatomic_int_get(spinlockAddr) == 1);
    uatomic_int_set(spinlockAddr, 0);
}


membuffer_pool membuffer_pool_create(uint32_t bsize, uint32_t numb)
{
    MBUF_Assert(bsize > 0 && bsize <= MBUF_SIZE_MAX);
    MBUF_Assert(numb > 0 && (bsize * numb) <= MBUF_POOL_SIZE_MAX);

    // 内存块大小是 MBUF_ALIGN_SIZE 字节的倍数
    const uint32_t bufferSize = (uint32_t)__align_size(bsize, MBUF_ALIGN_SIZE);

    // 内存块数量是 MBUF_FLAG_BITS(32) 的整数倍
    const uint32_t numBuffers = (uint32_t)__align_size(numb, MBUF_FLAG_BITS);

    if (bufferSize > MBUF_SIZE_MAX) {
        fprintf(stderr, "buffer size(=%zu) is more than MBUF_SIZE_MAX(=%zu).\n", (size_t) bufferSize, (size_t)MBUF_SIZE_MAX);
        return NULL;
    }

    const size_t poolSizeMax = bufferSize * numBuffers;
    if (poolSizeMax > MBUF_POOL_SIZE_MAX) {
        fprintf(stderr, "pool size(=%zu) is more than MBUF_POOL_SIZE_MAX(=%zu).\n", poolSizeMax, (size_t)MBUF_POOL_SIZE_MAX);
        return NULL;
    }

    // numFlags: 用多少个 flag 整数标记内存的状态位：1 空闲, 0 占用
    const uint32_t numFlags = numBuffers / MBUF_FLAG_BITS;

    size_t size = sizeof(membuffer_pool_t) + sizeof(membuffer_flag_t) * numFlags + poolSizeMax;

    membuffer_pool_t* pool = (membuffer_pool_t*)calloc(1, size);
    if (!pool) {
        return NULL;
    }

    pool->BufferSizeB = bufferSize;  // 修改只读字段（仅初始化时）
    pool->FlagsCount = numFlags;      // 修改只读字段（仅初始化时）

    pool->_Buffers = (unsigned char*)&pool->_Flags[numFlags];
    pool->unused_bits = MBUF_FLAG_BITS * pool->FlagsCount;

    // 设置全部块为空闲: flag = 1
    for (uint32_t i = 0; i < pool->FlagsCount; ++i) {
        pool->_Flags[i] = UINT32_MAX;
    }

    return pool;
}


void membuffer_pool_destroy(membuffer_pool pool)
{
    if (pool) {
        free(pool);
    }
}


void* membuffer_alloc(membuffer_pool pool, uint32_t bufferSize)
{
    // 需要的内存块数
    const int bitCount = (int)(__align_size(bufferSize + sizeof(membuffer_t), pool->BufferSizeB) / pool->BufferSizeB);

    // 计算最大可分配的容量
    if (bitCount == 0 || bitCount > MBUF_FLAG_BITS) {
        return NULL;  // 超过最大块
    }

    if (bitCount > pool->unused_bits) {
        return NULL;  // 空闲块不满足, 直接尝试下一个
    }

    __spinlock_grab(&pool->spinlock, MBUF_SPINS_MAX);

    membuffer_flag_t* pFlag;
    for (uint32_t flagOff = 0; flagOff < pool->FlagsCount; ++flagOff) {
        pFlag = pool->_Flags + flagOff;

        if (bitCount > MBUF_setbit_popcount(*pFlag)) {
            continue;  // 空闲块不满足, 直接尝试下一个
        }

        // startBit: 1-based
        int startBit = MBUF_first_setbit_n(*pFlag, bitCount);
        if (!startBit) {
            continue;  // 没有空闲块, 尝试下一个
        }

        // startBit 位偏移从 0 开始：0-based
        --startBit;

        // 计算块偏移以得到块的位置
        membuffer_t* pBuffer = (membuffer_t*)&pool->_Buffers[(flagOff * MBUF_FLAG_BITS + startBit) * pool->BufferSizeB];

        // 设置块头
        pBuffer->flagOffset = flagOff;
        pBuffer->bitOffset = (uint16_t)startBit;  // 块位偏移从 0 开始
        pBuffer->bitCount = (uint16_t)bitCount;   // 实际跨越的块数
        pBuffer->ownerPool = pool;

        // 清除块空闲位: 标记已经使用
        // pFlag 的从 startBit 开始的 bktCount 个位都要设置为 0
        if (bitCount == MBUF_FLAG_BITS) {
            *pFlag = 0;
        }
        else {
            // 生成掩码 (需要清除的位为0: BktCount 不能超过 31)
            membuffer_flag_t mask = ~(MASK_flag_table[bitCount] << startBit);
            *pFlag &= mask; // 清除对应的位
        }

        pool->unused_bits -= bitCount;

        __spinlock_free(&pool->spinlock);

        // 返回实际内存块的起始地址
        return (void*)pBuffer->_Memory;
    }

    // 不可达: 没有可用的块
    __spinlock_free(&pool->spinlock);

    return NULL;
}


void* membuffer_free(void* pMemory)
{
    MBUF_Assert(pMemory);
    if (!pMemory) {
        return NULL;
    }

    membuffer_t* pBuffer = (membuffer_t*)((char*)pMemory - offsetof(membuffer_t, _Memory));
    membuffer_pool pool = pBuffer->ownerPool;
    MBUF_Assert(pool);
    if (pool) {
        MBUF_Assert((char*)pBuffer >= (char*)pool->_Buffers &&
            (char*)pBuffer < (char*)pool->_Buffers + pool->BufferSizeB * pool->FlagsCount * MBUF_FLAG_BITS);

        if ((char*)pBuffer >= (char*)pool->_Buffers &&
            (char*)pBuffer < (char*)pool->_Buffers + pool->BufferSizeB * pool->FlagsCount * MBUF_FLAG_BITS) {
            membuffer_flag_t* pFlag = pool->_Flags + pBuffer->flagOffset;

            __spinlock_grab(&pool->spinlock, MBUF_SPINS_MAX);

            // 生成掩码：将 bitOffset 开始的 bitCount 个位设为 1，其余位不变
            membuffer_flag_t mask = (membuffer_flag_t)(MASK_flag_table[pBuffer->bitCount] << pBuffer->bitOffset);
            *pFlag |= mask; // 置位对应的位

            pool->unused_bits += (int) pBuffer->bitCount;

            __spinlock_free(&pool->spinlock);

            return NULL;
        }
    }
    return pMemory;
}


int membuffer_pool_stats(membuffer_pool pool, membuffer_stats_t* stats)
{
    if (stats) {
        stats->buffer_size = pool->BufferSizeB;
        stats->capacity_buffers = pool->FlagsCount * MBUF_FLAG_BITS;
    }

    return pool->unused_bits;
}
