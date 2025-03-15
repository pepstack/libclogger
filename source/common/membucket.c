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
** @file membucket.c
**   内存桶池实现，用于高效管理固定大小的内存块分配与释放。
** @author Liang Zhang <350137278@qq.com>
** @version 0.0.1
** @since 2025-03-14 02:54:00
** @date
*/
#include "membucket.h"

#include <stdlib.h>
#include <sched.h>    // sched_yield()

#include "timeut.h"   // 时间工具，用于退避等待
#include "uatomic.h"  // 原子操作库


// 自旋锁最大重试次数（指数退避）
#define SPINLOCKRETRY    10

// 桶字节尺寸对齐大小 = 128
#define MBKTALIGNSIZE   128

// 每个标志位管理的桶数（固定为32，对应uint32_t的位数）
#define MBKTFLAGBITS   ((uint32_t)(sizeof(uint32_t) * 8))

// 单个桶大小限制 8 MB
#define MBKTSIZEMAX      (MBKTALIGNSIZE * 8192 * 8)

// 内存池总大小限制 1024 MB (可能受实际可分配内存限制)
#define MBKPOOLSIZEMAX   (MBKTSIZEMAX * MBKTALIGNSIZE)

// 断言
#define MBKT_ASSERT(p)    assert(p)


// 内存桶头信息（位于每个分配块的首部）
typedef struct
{
    uint32_t flagOffset;  // 所属的flag结构在池中的索引
    uint16_t bitOffset;   // 在flag中的起始位索引（0-based）
    uint16_t bitCount;    // 占用的连续桶数
    char _Memory[0];      // 实际内存起始地址
} membucket_t;


// 内存池的标志结构（每个管理32个桶）
typedef struct
{
    uatomic_int spinlock; // 自旋锁
    uint32_t flag;        // 空闲位标志（1为空闲，0为占用）
} membucket_flag_t;


// 每个内存块大小固定：BktSize
// 每个内存块由一个标记位记录是1否0空闲
// 可供分配的内存 = BktSize x N
//   N = 1, 2, 3, ... 32
typedef struct membucket_pool_t
{
    uint32_t BktSize;    // size of a bucket in bytes
    uint32_t NumFlags;   // number of flags for all buckets
    unsigned char* _Buckets;
    membucket_flag_t  _Flags[0];
} membucket_pool_t;


// 预定义掩码表：bitCount=1~32 对应的掩码: 连续 bitCount 个1
static const uint32_t MASK_uint32_table[MBKTFLAGBITS + 1] = {
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

// 预定义查找表（保持不变）
static const char FFS_chars_table[256] = {
    0,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,
    6,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,
    7,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,
    6,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,
    8,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,
    6,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,
    7,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,
    6,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1,5,1,2,1,3,1,2,1,4,1,2,1,3,1,2,1
};


/**
 * @brief 获取自旋锁（强等待，指数退避后让出CPU）
 * @param spinlockAddr 自旋锁地址
 * @param usleepMax 最大退避微秒数
 */
static inline void spinlock_grab_strong(uatomic_int* spinlockAddr, const int usleepMax)
{
    int retry = 0;
    while (uatomic_int_comp_exch(spinlockAddr, 0, 1)) {
        if (retry < usleepMax) {
            sleep_usec(1 << retry);  // 指数退避
            ++retry;
        }
        else {
            // 让出 CPU
            sched_yield();
        }
    }
}


/**
 * @brief 对齐内存大小
 * @param size 原始大小
 * @param alignSize 对齐粒度
 * @return 对齐后的内存大小
 */
static inline size_t membucket_align_size(size_t size, size_t alignSize)
{
    return ((size + alignSize - 1) / alignSize) * alignSize;
}


/**
 * @brief 查找第一个置位（1-based）
 * @param flag 待查找的32位标志
 * @return 第一个置位的位置（1-based），全0返回0
 */
static inline int find_first_setbit_32(uint32_t flag)
{
    if (flag == 0) {
        return 0; // 全 0 直接返回
    }

    for (int i = 0; i < (int)sizeof(flag); ++i) {
        // 检查4个字节
        uint8_t byte = (flag >> (i * 8)) & 0xFF;
        if (byte != 0) {
            return i * 8 + FFS_chars_table[byte];
        }
    }

    return 0; // 理论上不可达
}

/**
 * @brief 从指定位置开始查找下一个置位（1-based）
 * @param flag 待查找的32位标志
 * @param startbit 起始位置（1-based）
 * @return 下一个置位的位置（1-based），未找到返回0
 */
static int find_next_setbit_32(uint32_t flag, int startbit)
{
    if (flag == 0) {
        return 0; // 全 0 直接返回
    }

    if (startbit < 2) {
        // 0, 1: 从头查找
        return find_first_setbit_32(flag);
    }

    // 处理 startbit 并转换为 0-based
    int start = startbit - 1;

    // 从起始字节开始遍历（0-based 字节索引：0~3）
    for (int byte_idx = start / 8; byte_idx < 4; ++byte_idx) {
        // 提取当前字节（0-based 字节内位：0~7）
        uint8_t byte = (flag >> (byte_idx * 8)) & 0xFF;
        if (byte == 0) {
            continue; // 跳过全 0 字节
        }

        int first_bit;
        if (byte_idx == start / 8) {
            // 处理起始字节：屏蔽起始位之前的位
            int shift = start % 8;
            uint8_t masked_byte = byte >> shift; // 右移舍去低位
            if (masked_byte == 0) {
                continue; // 起始字节剩余位全 0
            }
            // 查找第一个置位（masked_byte 的 1-based 位置）
            first_bit = FFS_chars_table[masked_byte];
            // 转换为原字节的 0-based 位置
            first_bit = first_bit + shift - 1;
        }
        else {
            // 非起始字节：直接查找第一个置位
            first_bit = FFS_chars_table[byte] - 1; // 0-based
        }

        // 计算全局位（0-based）
        int global_bit = byte_idx * 8 + first_bit;
        if (global_bit >= start) {
            return global_bit + 1; // 返回 1-based 位置
        }
    }

    return 0; // 未找到
}

/**
 * @brief 查找第一个清0位（1-based）
 * @param flag 待查找的32位标志
 * @return 第一个清0位的位置（1-based），全0返回0
 */
static int find_next_unsetbit_32(uint32_t flag, int startbit)
{
    const int start = startbit - 1;  // 转换为0-based
    uint32_t mask = (start >= 32) ? 0 : (0xFFFFFFFFU << start); // 安全掩码
    uint32_t inverted = (~flag) & mask;

    if (inverted == 0) return 0;

    // 从起始字节开始逐字节查找
    for (int byte_offset = (start / 8) * 8; byte_offset < 32; byte_offset += 8) {
        uint8_t byte = (inverted >> byte_offset) & 0xFF;
        if (byte != 0) {
            int bit_in_byte = FFS_chars_table[byte] - 1; // 转0-based
            int global_bit = byte_offset + bit_in_byte;
            return (global_bit < 32) ? (global_bit + 1) : 0; // 返回1-based
        }
    }
    return 0;
}

/**
 * @brief 查找连续 count 个置位的起始位置（1-based）
 * @param flag 待查找的32位标志
 * @param count 连续置位的数量
 * @return 起始位置（1-based），未找到返回0
 */
static int find_first_setbit_count_32(uint32_t flag, int count)
{
    // assert(count > 0 && count <= 32);
    // 特殊优化：count=1 直接复用 find_first_setbit_32
    if (count == 1) {
        return find_first_setbit_32(flag);
    }

    // 特殊优化：count=32 直接判断全1
    if (count == 32) {
        return (flag == 0xFFFFFFFF) ? 1 : 0;
    }

    int current_start = 1; // 起始位（1-based）

    while (1) {
        // 1. 找到下一个置位的位置
        int pos = find_next_setbit_32(flag, current_start);
        if (pos == 0) return 0; // 没有更多置位

        // 2. 找到下一个清零位的位置（从 pos 开始）
        int end_pos = find_next_unsetbit_32(flag, pos);

        // 3. 处理 end_pos=0 的情况（剩余位全1）
        if (end_pos == 0) {
            end_pos = 32 + 1; // 总位数是32，end_pos=33表示最后一个位的下一个位置
        }

        // 4. 计算连续置位长度
        int available = end_pos - pos;

        // 5. 检查是否满足长度要求
        if (available >= count) {
            return pos;
        }

        // 6. 更新起始位置为 end_pos，继续查找
        current_start = end_pos;
    }
}
// 自旋锁最大重试次数（指数退避）
#define SPINLOCKRETRY    10

// 桶字节尺寸对齐大小 = 128
#define MBKTALIGNSIZE   128

// 每个标志位管理的桶数（固定为32，对应uint32_t的位数）
#define MBKTFLAGBITS   ((uint32_t)(sizeof(uint32_t) * 8))

// 单个桶大小限制 8 MB
#define MBKTSIZEMAX      (MBKTALIGNSIZE * 8192 * 8)

// 内存池总大小限制 1024 MB (可能受实际可分配内存限制)
#define MBKPOOLSIZEMAX   (MBKTSIZEMAX * MBKTALIGNSIZE)

membucket_pool membucket_pool_create(uint32_t bktsize, uint32_t numbkts)
{
    MBKT_ASSERT(MBKTFLAGBITS == 32);                       // 确保标志位数正确
    MBKT_ASSERT(MBKTALIGNSIZE >= sizeof(membucket_t));     // 对齐需容纳头结构
    MBKT_ASSERT(MBKTALIGNSIZE % sizeof(membucket_t) == 0); // 对齐是头大小的整数倍

    MBKT_ASSERT(bktsize > 0 && bktsize <= MBKTSIZEMAX);
    MBKT_ASSERT(numbkts > 0 && (bktsize * numbkts) <= MBKPOOLSIZEMAX);

    // 内存桶是 128 字节的倍数
    const uint32_t bsize = (uint32_t)membucket_align_size(bktsize, MBKTALIGNSIZE);

    // 桶数量是 32 的整数倍
    const uint32_t numb = (uint32_t)membucket_align_size(numbkts, MBKTFLAGBITS);

    if (bsize > MBKTSIZEMAX) {
        fprintf(stderr, "bucket size(=%zu) is more than MBKTSIZEMAX(=%zu).\n", (size_t) bsize, (size_t)MBKTSIZEMAX);
        return NULL;
    }

    const size_t poolSizeMax = bsize * numb;
    if (poolSizeMax > MBKPOOLSIZEMAX) {
        fprintf(stderr, "pool size(=%zu) is more than MBKPOOLSIZEMAX(=%zu).\n", poolSizeMax, (size_t)MBKPOOLSIZEMAX);
        return NULL;
    }

    // flags: 用多少个 32 位整数标记桶的状态位：1 空闲, 0 占用
    const uint32_t flags = numb / MBKTFLAGBITS;

    size_t size = sizeof(membucket_pool_t) + sizeof(membucket_flag_t) * flags + poolSizeMax;

    membucket_pool_t* pool = (membucket_pool_t*)calloc(1, size);
    if (!pool) {
        return NULL;
    }

    pool->BktSize = bsize;  // 修改只读字段（仅初始化时）
    pool->NumFlags = flags; // 修改只读字段（仅初始化时）

    pool->_Buckets = (unsigned char*)&pool->_Flags[flags];

    // 设置全部块为空闲: flag = 1
    for (uint32_t i = 0; i < pool->NumFlags; ++i) {
        pool->_Flags[i].spinlock = 0;
        pool->_Flags[i].flag = 0xFFFFFFFF;
    }

    return pool;
}


void membucket_pool_destroy(membucket_pool pool)
{
    if (pool) {
        free(pool);
    }
}


void* membucket_pool_alloc(membucket_pool pool, uint32_t bsize)
{
    const size_t BktSize = pool->BktSize;

    // 需要的桶数
    const uint32_t bitCount = (uint32_t)(membucket_align_size(bsize + sizeof(membucket_t), BktSize) / BktSize);

    // 计算最大可分配的容量
    if (bitCount == 0 || bitCount > MBKTFLAGBITS) {
        return NULL;  // 超过最大可分配的桶
    }

    membucket_flag_t* pBmp;
    for (uint32_t flagOff = 0; flagOff < pool->NumFlags; ++flagOff) {
        pBmp = pool->_Flags + flagOff;

        if (!pBmp->flag) {
            continue;  // 没有空闲桶, 直接尝试下一个
        }

        // 一直等待自旋锁释放
        spinlock_grab_strong(&pBmp->spinlock, SPINLOCKRETRY);

        // 自此取得自旋锁
        int startBit = find_first_setbit_count_32(pBmp->flag, bitCount);
        if (!startBit) {
            uatomic_int_set(&pBmp->spinlock, 0);  // 释放自旋锁
            continue;  // 没有空闲桶, 尝试下一个
        }

        // startBit 位偏移从 0 开始：0-based
        --startBit;

        // 计算桶偏移以得到桶的位置
        membucket_t* pBucket = (membucket_t*)&pool->_Buckets[(flagOff * MBKTFLAGBITS + startBit) * BktSize];

        // 设置桶头
        pBucket->flagOffset = flagOff;
        pBucket->bitOffset = (uint16_t)startBit;  // 桶位偏移从 0 开始
        pBucket->bitCount = bitCount;              // 实际跨越的桶数

        // 清除桶空闲位: 标记已经使用
        // pBmp->flag 的从 startBit 开始的 bktCount 个位都要设置为 0
        if (bitCount == MBKTFLAGBITS) {
            pBmp->flag = 0;
        }
        else {
            // 生成掩码 (需要清除的位为0: BktCount 不能超过 31)
            uint32_t mask = ~(MASK_uint32_table[bitCount] << startBit);
            pBmp->flag &= mask; // 清除对应的位
        }

        uatomic_int_set(&pBmp->spinlock, 0);  // 释放自旋锁

        // 返回实际内存块的起始地址
        return (void*)pBucket->_Memory;
    }

    // 分配失败: 没有可用的桶
    return NULL;
}


void membucket_pool_free(membucket_pool pool, void* pMemory)
{
    if (pMemory) {
        membucket_t* pBucket = (membucket_t*)pMemory;
        --pBucket;

        MBKT_ASSERT((char*)pBucket >= (char*)pool->_Buckets &&
            (char*)pBucket < (char*)pool->_Buckets + pool->BktSize * pool->NumFlags * MBKTFLAGBITS);

        membucket_flag_t* pBmp = pool->_Flags + pBucket->flagOffset;

        // 一直等待自旋锁释放
        spinlock_grab_strong(&pBmp->spinlock, SPINLOCKRETRY);

        // 生成掩码：将 bitOffset 开始的 bitCount 个位设为 1，其余位不变
        uint32_t mask = (uint32_t)(MASK_uint32_table[pBucket->bitCount] << pBucket->bitOffset);
        pBmp->flag |= mask; // 置位对应的位

        uatomic_int_set(&pBmp->spinlock, 0);
    }
}