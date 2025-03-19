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
** @file ffs64.h
**   find first set for 64-bits int
** @author Liang Zhang <350137278@qq.com>
** @version 0.0.1
** @since 2025-03-14 02:54:00
** @date
*/
#ifndef FFS64_H_
#define FFS64_H_

#include <stdint.h>
#include <assert.h>

#if defined(_MSC_VER)
  // MSVC 平台
  #include <intrin.h>
  #define FFS64_HAS_BitScanForward
#elif defined(__GNUC__) || defined(__clang__)
  // GCC/Clang 平台
  #define FFS64_HAS__builtin_ffs
#else
  // 其他平台回退实现
  #undef FFS64_HAS_BitScanForward
  #undef FFS64_HAS__builtin_ffs
#endif

#define FFS64_FLAG_BITS   64
#define FFS64_FLAG_MAX    UINT64_MAX

typedef uint64_t ffs64_flag_t;


// 静态断言
#define FFS64_StaticAssert(cond, msg) \
    typedef char __FFS64_StaticAssert_##msg[(cond) ? 1 : -1]

// 断言
#define FFS64_Assert(p) assert((p) && "FFS64 Assertion failed: " #p)

#ifdef __cplusplus
extern "C" {
#endif

// 确保 ffs64_flag_t 为 8 字节
FFS64_StaticAssert(sizeof(ffs64_flag_t)*8 == FFS64_FLAG_BITS, ffs64_flag_must_be_64_bits);

/**
 * @brief 查找第一个置位（1-based）
 * @param flag 待查找的64位标志
 * @return 第一个置位的位置（1-based），全0返回0
 */
static inline int FFS64_first_setbit(ffs64_flag_t flag)
{
    if (flag == 0) {
        return 0;
    }
    if (flag == UINT64_MAX) {
        return 1;
    }
#if defined(FFS64_HAS_BitScanForward)
    unsigned long index;
    return _BitScanForward64(&index, flag) ? (int)(index + 1) : 0;
#elif defined(FFS64_HAS__builtin_ffs)
    return __builtin_ffsll(flag);
#else
    // De Bruijn 序列实现
    static const int debruijn64_table[FFS64_FLAG_BITS] = {
        0,  1, 48,  2, 57, 49, 28,  3,
        61, 58, 50, 42, 38, 29, 17,  4,
        62, 55, 59, 36, 53, 51, 43, 22,
        45, 39, 33, 30, 24, 18, 12,  5,
        63, 47, 56, 27, 60, 41, 37, 16,
        54, 35, 52, 21, 44, 32, 23, 11,
        46, 26, 40, 15, 34, 20, 31, 10,
        25, 14, 19,  9, 13,  8,  7,  6
    };
    static const ffs64_flag_t debruijn64 = 0x03F79D71B4CB0A89ULL;
    return debruijn64_table[((flag & (~flag + 1)) * debruijn64) >> 58] + 1;
#endif
}

/**
 * @brief 查找最后一个置位（1-based）
 * @param flag 待查找的64位标志
 * @return 最后一个置位的位置（1-based），全0返回0
 */
static int FFS64_last_setbit(ffs64_flag_t flag)
{
    if (flag == 0) {
        return 0;
    }
    if (flag == FFS64_FLAG_MAX) {
        return FFS64_FLAG_BITS; // 全1时最高位是64
    }

#if defined(_MSC_VER)
    // MSVC 平台优化（需支持64位指令集）
    unsigned long index;
    return _BitScanReverse64(&index, flag) ? (int)(index + 1) : 0;

#elif defined(__GNUC__) || defined(__clang__)
    // GCC/Clang 平台优化
    return flag ? (FFS64_FLAG_BITS - __builtin_clzll(flag)) : 0;
#else
    // 通用位操作实现（无编译器内置函数时）
    // 原理：通过位填充找到最高有效位
    flag |= flag >> 1;
    flag |= flag >> 2;
    flag |= flag >> 4;
    flag |= flag >> 8;
    flag |= flag >> 16;
    flag |= flag >> 32; // 新增的64位处理步骤
    flag = (flag >> 1) + 1; // 保留最高位

    // 使用64位 De Bruijn 序列查找
    static const int debruijn64_table[FFS64_FLAG_BITS] = {
        0,  1, 48,  2, 57, 49, 28,  3,
        61, 58, 50, 42, 38, 29, 17,  4,
        62, 55, 59, 36, 53, 51, 43, 22,
        45, 39, 33, 30, 24, 18, 12,  5,
        63, 47, 56, 27, 60, 41, 37, 16,
        54, 35, 52, 21, 44, 32, 23, 11,
        46, 26, 40, 15, 34, 20, 31, 10,
        25, 14, 19,  9, 13,  8,  7,  6
    };
    static const ffs64_flag_t debruijn64 = 0x03F79D71B4CB0A89ULL;
    return debruijn64_table[(flag * debruijn64) >> 58] + 1;
#endif
}

/**
 * @brief 查找连续 n 个置位的起始位置（1-based）
 * @param flag 待查找的64位标志
 * @param n 连续置位的数量
 * @return 起始位置（1-based），未找到返回0
 */
static inline int FFS64_first_setbit_n(ffs64_flag_t flag, int n)
{
    FFS64_Assert(n > 0 && n <= FFS64_FLAG_BITS);
    if (n == 1) {
        return FFS64_first_setbit(flag);
    }
    if (n == FFS64_FLAG_BITS) {
        return (flag == UINT64_MAX) ? 1 : 0;
    }
    const ffs64_flag_t mask = (1ULL << n) - 1;
    for (int shift = 0; shift <= FFS64_FLAG_BITS - n; ++shift) {
        if ((flag & (mask << shift)) == (mask << shift)) {
            return shift + 1;
        }
    }
    return 0;
}

/**
 * @brief 从指定位置开始查找下一个置位（1-based）
 * @param flag 待查找的64位标志
 * @param startbit 起始位置（1-based）
 * @return 下一个置位的位置（1-based），未找到返回0
 */
static inline int FFS64_next_setbit(ffs64_flag_t flag, int startbit)
{
    FFS64_Assert(startbit > 0 && startbit <= FFS64_FLAG_BITS);
    if (flag == 0) {
        return 0;
    }
    const int start0 = startbit - 1;
    const ffs64_flag_t masked = flag >> start0;
    const int pos = FFS64_first_setbit(masked);
    return pos ? (pos + start0) : 0;
}

/**
 * @brief 从指定位置开始查找下一个置0位（1-based）
 * @param flag 待查找的64位标志
 * @param startbit 起始位置（1-based）
 * @return 下一个置0位的位置（1-based），未找到返回0
 */
static inline int FFS64_next_unsetbit(ffs64_flag_t flag, int startbit)
{
    FFS64_Assert(startbit > 0 && startbit <= FFS64_FLAG_BITS);
    if (flag == UINT64_MAX) {
        return 0;
    }
    const int start0 = startbit - 1;
    const ffs64_flag_t inverted = ~flag;
    const ffs64_flag_t masked = inverted >> start0;
    const int pos = FFS64_first_setbit(masked);
    return pos ? (pos + start0) : 0;
}

/**
 * @brief 计算64位整数中置位的个数
 * @param flag 待检查的64位标志
 * @return 置位的个数: [0,64]
 */
static inline int FFS64_setbit_popcount(ffs64_flag_t flag)
{
    if (flag == 0) {
        return 0;
    }
    if (flag == UINT64_MAX) {
        return FFS64_FLAG_BITS;
    }
#if defined(__GNUC__) || defined(__clang__)
    return (int)__builtin_popcountll(flag);
#elif defined(_MSC_VER)
    return (int)__popcnt64(flag);
#else
    flag = flag - ((flag >> 1) & 0x5555555555555555ULL);
    flag = (flag & 0x3333333333333333ULL) + ((flag >> 2) & 0x3333333333333333ULL);
    flag = (flag + (flag >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return (int)((flag * 0x0101010101010101ULL) >> 56);
#endif
}

#ifdef __cplusplus
}
#endif
#endif /* FFS64_H_ */