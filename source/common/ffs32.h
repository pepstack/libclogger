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
** @file ffs32.h
**   find first set for 32-bits int
** @author Liang Zhang <350137278@qq.com>
** @version 0.0.1
** @since 2025-03-14 02:54:00
** @date
*/
#include <stdint.h>

#ifndef FFS32_H_
#define FFS32_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


// 分段预定义查找表
static const char FFS32_setbits_table[256] = {
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
 * @brief 查找第一个置位（1-based）
 * @param flag 待查找的32位标志
 * @return 第一个置位的位置（1-based），全0返回0
 */
static inline int FFS_first_setbit_32(uint32_t flag)
{
    if (flag == 0) {
        return 0; // 全 0 直接返回
    }
    for (int i = 0; i < (int)sizeof(flag); ++i) {
        // 检查4个字节
        uint8_t byte = (flag >> (i * 8)) & 0xFF;
        if (byte != 0) {
            return i * 8 + FFS32_setbits_table[byte];
        }
    }
    return 0; // 理论上不可达
}


/**
 * @brief 查找置位的个数
 * @param flag 待查找的32位标志
 * @return 置位的个数: [0,32]
 */
static inline uint32_t FFS_setbit_count_32(uint32_t flag)
{
    if (!flag) return 0;
    if (flag == 0xFFFFFFFF) return 32;
    flag = flag - ((flag >> 1) & 0x55555555);
    flag = (flag & 0x33333333) + ((flag >> 2) & 0x33333333);
    flag = (flag + (flag >> 4)) & 0x0F0F0F0F;
    flag = (flag * 0x01010101) >> 24;
    return flag;
}


/**
 * @brief 从指定位置开始查找下一个置位（1-based）
 * @param flag 待查找的32位标志
 * @param startbit 起始位置（1-based）
 * @return 下一个置位的位置（1-based），未找到返回0
 */
static int FFS_next_setbit_32(uint32_t flag, int startbit)
{
    if (flag == 0) {
        return 0; // 全 0 直接返回
    }

    if (startbit < 2) {
        // 0, 1: 从头查找
        return FFS_first_setbit_32(flag);
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
            first_bit = FFS32_setbits_table[masked_byte];
            // 转换为原字节的 0-based 位置
            first_bit = first_bit + shift - 1;
        }
        else {
            // 非起始字节：直接查找第一个置位
            first_bit = FFS32_setbits_table[byte] - 1; // 0-based
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
static int FFS_next_unsetbit_32(uint32_t flag, int startbit)
{
    const int start = startbit - 1;  // 转换为0-based
    uint32_t mask = (start >= 32) ? 0 : (0xFFFFFFFFU << start); // 安全掩码
    uint32_t inverted = (~flag) & mask;

    if (inverted == 0) return 0;

    // 从起始字节开始逐字节查找
    for (int byte_offset = (start / 8) * 8; byte_offset < 32; byte_offset += 8) {
        uint8_t byte = (inverted >> byte_offset) & 0xFF;
        if (byte != 0) {
            int bit_in_byte = FFS32_setbits_table[byte] - 1; // 转0-based
            int global_bit = byte_offset + bit_in_byte;
            return (global_bit < 32) ? (global_bit + 1) : 0; // 返回1-based
        }
    }
    return 0;
}


/**
 * @brief 查找连续 num 个置位的起始位置（1-based）
 * @param flag 待查找的32位标志
 * @param count 连续置位的数量
 * @return 起始位置（1-based），未找到返回0
 */
static int FFS_first_setbit_n_32(uint32_t flag, int num)
{
    // assert(num > 0 && num <= 32);
    // 特殊优化：num=1 直接复用 find_first_setbit_32
    if (num == 1) {
        return FFS_first_setbit_32(flag);
    }

    // 特殊优化：num=32 直接判断全1
    if (num == 32) {
        return (flag == 0xFFFFFFFF) ? 1 : 0;
    }

    int current_start = 1; // 起始位（1-based）

    while (1) {
        // 1. 找到下一个置位的位置
        int pos = FFS_next_setbit_32(flag, current_start);
        if (pos == 0) return 0; // 没有更多置位

        // 2. 找到下一个清零位的位置（从 pos 开始）
        int end_pos = FFS_next_unsetbit_32(flag, pos);

        // 3. 处理 end_pos=0 的情况（剩余位全1）
        if (end_pos == 0) {
            end_pos = 32 + 1; // 总位数是32，end_pos=33表示最后一个位的下一个位置
        }

        // 4. 计算连续置位长度
        int available = end_pos - pos;

        // 5. 检查是否满足长度要求
        if (available >= num) {
            return pos;
        }

        // 6. 更新起始位置为 end_pos，继续查找
        current_start = end_pos;
    }
}

#ifdef    __cplusplus
}
#endif
#endif /* FFS32_H_ */