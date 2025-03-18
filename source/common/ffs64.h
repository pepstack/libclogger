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

// 静态断言
#define FFS64_StaticAssert(cond, msg) \
    typedef char __FFS64_StaticAssert_##msg[(cond) ? 1 : -1]

// 断言
#define FFS64_Assert(p) assert((p) && "Assertion failed: " #p)

#ifdef __cplusplus
extern "C" {
#endif

// 确保 uint64_t 为 8 字节
FFS64_StaticAssert(sizeof(uint64_t) == 8, uint64_t_must_be_8_bytes);

/**
 * @brief 查找第一个置位（1-based）
 * @param flag 待查找的64位标志
 * @return 第一个置位的位置（1-based），全0返回0
 */
static inline int FFS64_first_setbit(uint64_t flag)
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
    static const int debruijn_table[64] = {
        0,  1, 48,  2, 57, 49, 28,  3,
        61, 58, 50, 42, 38, 29, 17,  4,
        62, 55, 59, 36, 53, 51, 43, 22,
        45, 39, 33, 30, 24, 18, 12,  5,
        63, 47, 56, 27, 60, 41, 37, 16,
        54, 35, 52, 21, 44, 32, 23, 11,
        46, 26, 40, 15, 34, 20, 31, 10,
        25, 14, 19,  9, 13,  8,  7,  6
    };
    const uint64_t debruijn = 0x03F79D71B4CB0A89ULL;
    return debruijn_table[((flag & (~flag + 1)) * debruijn) >> 58] + 1;
#endif
}


/**
 * @brief 查找连续 n 个置位的起始位置（1-based）
 * @param flag 待查找的64位标志
 * @param n 连续置位的数量
 * @return 起始位置（1-based），未找到返回0
 */
static inline int FFS64_first_setbit_n(uint64_t flag, int n)
{
    FFS64_Assert(n > 0 && n <= 64);
    if (n == 1) {
        return FFS64_first_setbit(flag);
    }
    if (n == 64) {
        return (flag == UINT64_MAX) ? 1 : 0;
    }
    const uint64_t mask = (1ULL << n) - 1;
    for (int shift = 0; shift <= 64 - n; ++shift) {
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
static inline int FFS64_next_setbit(uint64_t flag, int startbit)
{
    FFS64_Assert(startbit > 0 && startbit <= 64);
    if (flag == 0) {
        return 0;
    }
    const int start0 = startbit - 1;
    const uint64_t masked = flag >> start0;
    const int pos = FFS64_first_setbit(masked);
    return pos ? (pos + start0) : 0;
}

/**
 * @brief 从指定位置开始查找下一个置0位（1-based）
 * @param flag 待查找的64位标志
 * @param startbit 起始位置（1-based）
 * @return 下一个置0位的位置（1-based），未找到返回0
 */
static inline int FFS64_next_unsetbit(uint64_t flag, int startbit)
{
    FFS64_Assert(startbit > 0 && startbit <= 64);
    if (flag == UINT64_MAX) {
        return 0;
    }
    const int start0 = startbit - 1;
    const uint64_t inverted = ~flag;
    const uint64_t masked = inverted >> start0;
    const int pos = FFS64_first_setbit(masked);
    return pos ? (pos + start0) : 0;
}

/**
 * @brief 计算64位整数中置位的个数
 * @param flag 待检查的64位标志
 * @return 置位的个数: [0,64]
 */
static inline uint64_t FFS64_setbit_popcount(uint64_t flag)
{
    if (flag == 0) {
        return 0;
    }
    if (flag == UINT64_MAX) {
        return 64;
    }
#if defined(__GNUC__) || defined(__clang__)
    return __builtin_popcountll(flag);
#elif defined(_MSC_VER)
    return __popcnt64(flag);
#else
    flag = flag - ((flag >> 1) & 0x5555555555555555ULL);
    flag = (flag & 0x3333333333333333ULL) + ((flag >> 2) & 0x3333333333333333ULL);
    flag = (flag + (flag >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
    return (flag * 0x0101010101010101ULL) >> 56;
#endif
}

#ifdef __cplusplus
}
#endif
#endif /* FFS64_H_ */