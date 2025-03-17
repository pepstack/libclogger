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
#include "ffs32.h"    // FindFirstSet

#include <assert.h>  // assert()
#include <sched.h>   // sched_yield()


// 断言
#define MBUF_ASSERT(p)    assert(p)


// 自旋锁最大重试次数（指数退避）: 10 是比较好的
#define MBUF_SPINS_MAX       10

// 内存块字节尺寸对齐大小 = 128
#define MBUF_ALIGN_SIZE      128

// 每个标志位管理的内存块数（固定为32，对应uint32_t的位数）
#define MBUF_FLAG_BITS       ((uint32_t)(sizeof(uint32_t) * 8))

// 单个内存块大小限制 8 MB
#define MBUF_SIZE_MAX        (MBUF_ALIGN_SIZE * 8192 * 8)

// 内存池总大小限制 1024 MB (可能受实际可分配内存限制)
#define MBUF_POOL_SIZE_MAX   (MBUF_SIZE_MAX * MBUF_ALIGN_SIZE)


// 内存池的标志结构（每个结构管理32个内存块）
typedef struct
{
    uatomic_int spinlock; // 自旋锁
    uint32_t bitflag;     // bitflag: 空闲位标志（1为空闲，0为占用）
} membuffer_flag_t;


// 每个内存块大小固定：BufferSizeB
// 每个内存块由一个标记位记录是1否0空闲
// 可供分配的内存 = BufferSizeB x N
//   N = 1, 2, 3, ... 32
typedef struct membuffer_pool_t
{
    uint32_t BufferSizeB;     // size in bytes of per buffer
    uint32_t BuffersCount;    // count of Flags/Buffers

    unsigned char* _Buffers;
    membuffer_flag_t  _Flags[0];
} membuffer_pool_t;


// 内存头信息（位于每个分配块的首部）
typedef struct
{
    membuffer_pool_t* ownerPool; // 所属的内存池
    uint32_t flagOffset;         // 所属的flag结构在池中的索引
    uint16_t bitOffset;          // 在flag中的起始位索引（0-based）
    uint16_t bitCount;           // 占用的连续内存块数（1bit对应一个内存块）
    char _Memory[0];             // 实际内存起始地址
} membuffer_t;


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
    MBUF_ASSERT(uatomic_int_get(spinlockAddr) == 1);
    uatomic_int_set(spinlockAddr, 0);
}


// 获取自旋锁
#define MBUF_SPINLOCK_GRAB(pOwner)   __spinlock_grab(&pOwner->spinlock, MBUF_SPINS_MAX)

// 释放自旋锁
#define MBUF_SPINLOCK_FREE(pOwner)   __spinlock_free(&pOwner->spinlock)


// 预定义掩码表：bitCount=1~32 对应的掩码: 连续 bitCount 个1
static const uint32_t MASK_uint32_table[MBUF_FLAG_BITS + 1] = {
    0x00000000, // bitCount=0（占位，实际不使用）
    0x00000001, // bitCount=1: 0b000...0001
    0x00000003, // bitCount=2: 0b000...0011
    0x00000007, // bitCount=3: 0b000...0111
    0x0000000F, // bitCount=4: 0b000...1111
    0x0000001F, // bitCount=5: 0b000...0001 1111
    0x0000003F, // bitCount=6: 0b000...0011 1111
    0x0000007F, // bitCount=7: 0b000...0111 1111
    0x000000FF, // bitCount=8: 0b000...1111 1111
    0x000001FF, // bitCount=9: 0b0001 1111 1111
    0x000003FF, // bitCount=10: 0b0011 1111 1111
    0x000007FF, // bitCount=11: 0b0111 1111 1111
    0x00000FFF, // bitCount=12: 0b1111 1111 1111
    0x00001FFF, // bitCount=13
    0x00003FFF, // bitCount=14
    0x00007FFF, // bitCount=15
    0x0000FFFF, // bitCount=16
    0x0001FFFF, // bitCount=17
    0x0003FFFF, // bitCount=18
    0x0007FFFF, // bitCount=19
    0x000FFFFF, // bitCount=20
    0x001FFFFF, // bitCount=21
    0x003FFFFF, // bitCount=22
    0x007FFFFF, // bitCount=23
    0x00FFFFFF, // bitCount=24
    0x01FFFFFF, // bitCount=25
    0x03FFFFFF, // bitCount=26
    0x07FFFFFF, // bitCount=27
    0x0FFFFFFF, // bitCount=28
    0x1FFFFFFF, // bitCount=29
    0x3FFFFFFF, // bitCount=30
    0x7FFFFFFF, // bitCount=31
    0xFFFFFFFF  // bitCount=32: 全1
};


membuffer_pool membuffer_pool_create(uint32_t bsize, uint32_t numb)
{
    MBUF_ASSERT(MBUF_FLAG_BITS == 32);                       // 确保标志位数正确
    MBUF_ASSERT(MBUF_ALIGN_SIZE % MBUF_FLAG_BITS == 0);      // 对齐需容纳头结构
    MBUF_ASSERT(MBUF_ALIGN_SIZE % sizeof(membuffer_t) == 0); // 对齐是头大小的整数倍

    MBUF_ASSERT(bsize > 0 && bsize <= MBUF_SIZE_MAX);
    MBUF_ASSERT(numb > 0 && (bsize * numb) <= MBUF_POOL_SIZE_MAX);

    // 内存块大小是 MBUF_ALIGN_SIZE(128) 字节的倍数
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

    // numFlags: 用多少个 32 位整数标记内存的状态位：1 空闲, 0 占用
    const uint32_t numFlags = numBuffers / MBUF_FLAG_BITS;

    size_t size = sizeof(membuffer_pool_t) + sizeof(membuffer_flag_t) * numFlags + poolSizeMax;

    membuffer_pool_t* pool = (membuffer_pool_t*)calloc(1, size);
    if (!pool) {
        return NULL;
    }

    pool->BufferSizeB = bufferSize;  // 修改只读字段（仅初始化时）
    pool->BuffersCount = numFlags;      // 修改只读字段（仅初始化时）

    pool->_Buffers = (unsigned char*)&pool->_Flags[numFlags];

    // 设置全部块为空闲: flag = 1
    for (uint32_t i = 0; i < pool->BuffersCount; ++i) {
        pool->_Flags[i].spinlock = 0;
        pool->_Flags[i].bitflag = 0xFFFFFFFF;
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
    const uint32_t bitCount = (uint32_t)(__align_size(bufferSize + sizeof(membuffer_t), pool->BufferSizeB) / pool->BufferSizeB);

    // 计算最大可分配的容量
    if (bitCount == 0 || bitCount > MBUF_FLAG_BITS) {
        return NULL;  // 超过最大可分配的块
    }

    membuffer_flag_t* pFlag;
    for (uint32_t flagOff = 0; flagOff < pool->BuffersCount; ++flagOff) {
        pFlag = pool->_Flags + flagOff;

        MBUF_SPINLOCK_GRAB(pFlag);

        if (bitCount > FFS_setbit_count_32(pFlag->bitflag)) {
            MBUF_SPINLOCK_FREE(pFlag);
            continue;  // 空闲块不满足, 直接尝试下一个
        }

        // 自此取得自旋锁
        int startBit = FFS_first_setbit_n_32(pFlag->bitflag, bitCount);
        if (!startBit) {
            MBUF_SPINLOCK_FREE(pFlag);
            continue;  // 没有空闲块, 尝试下一个
        }

        // startBit 位偏移从 0 开始：0-based
        --startBit;

        // 计算块偏移以得到块的位置
        membuffer_t* pBuffer = (membuffer_t*)&pool->_Buffers[(flagOff * MBUF_FLAG_BITS + startBit) * pool->BufferSizeB];

        // 设置块头
        pBuffer->flagOffset = flagOff;
        pBuffer->bitOffset = (uint16_t)startBit;  // 块位偏移从 0 开始
        pBuffer->bitCount = bitCount;             // 实际跨越的块数
        pBuffer->ownerPool = pool;

        // 清除块空闲位: 标记已经使用
        // pFlag->bitflag 的从 startBit 开始的 bktCount 个位都要设置为 0
        if (bitCount == MBUF_FLAG_BITS) {
            pFlag->bitflag = 0;
        }
        else {
            // 生成掩码 (需要清除的位为0: BktCount 不能超过 31)
            uint32_t mask = ~(MASK_uint32_table[bitCount] << startBit);
            pFlag->bitflag &= mask; // 清除对应的位
        }

        MBUF_SPINLOCK_FREE(pFlag);

        // 返回实际内存块的起始地址
        return (void*)pBuffer->_Memory;
    }

    // 不可达: 没有可用的块
    return NULL;
}


void* membuffer_free(void* pMemory)
{
    MBUF_ASSERT(pMemory);

    if (!pMemory) {
        return NULL;
    }

    membuffer_t* pBuffer = (membuffer_t*)pMemory;
    --pBuffer;

    membuffer_pool pool = pBuffer->ownerPool;
    MBUF_ASSERT(pool);

    MBUF_ASSERT((char*)pBuffer >= (char*)pool->_Buffers &&
        (char*)pBuffer < (char*)pool->_Buffers + pool->BufferSizeB * pool->BuffersCount * MBUF_FLAG_BITS);

    if ((char*)pBuffer >= (char*)pool->_Buffers &&
        (char*)pBuffer < (char*)pool->_Buffers + pool->BufferSizeB * pool->BuffersCount * MBUF_FLAG_BITS) {
        membuffer_flag_t* pFlag = pool->_Flags + pBuffer->flagOffset;

        MBUF_SPINLOCK_GRAB(pFlag);

        // 生成掩码：将 bitOffset 开始的 bitCount 个位设为 1，其余位不变
        uint32_t mask = (uint32_t)(MASK_uint32_table[pBuffer->bitCount] << pBuffer->bitOffset);
        pFlag->bitflag |= mask; // 置位对应的位

        MBUF_SPINLOCK_FREE(pFlag);

        return NULL;
    }

    return pMemory;
}


uint32_t membuffer_pool_stats(membuffer_pool pool, membuffer_stats_t* stats)
{
    uint32_t unused_buckets = 0;

    membuffer_flag_t* pFlag;
    for (uint32_t flagOff = 0; flagOff < pool->BuffersCount; ++flagOff) {
        pFlag = pool->_Flags + flagOff;

        MBUF_SPINLOCK_GRAB(pFlag);

        unused_buckets += FFS_setbit_count_32(pFlag->bitflag);

        MBUF_SPINLOCK_FREE(pFlag);
    }

    if (stats) {
        stats->buffer_size = pool->BufferSizeB;
        stats->capacity_buffers = pool->BuffersCount * MBUF_FLAG_BITS;
    }

    return unused_buckets;
}
