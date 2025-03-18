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
** @file ffs32.h
**   find first set for 32-bits int
** @author Liang Zhang <350137278@qq.com>
** @version 0.0.1
** @since 2025-03-14 02:54:00
** @date 2025-03-19 00:04:00
**
*/
#ifndef FFS32_H_
#define FFS32_H_

#include <stdint.h>
#include <assert.h>

#if defined(_MSC_VER)
  // MSVC 平台
  #include <intrin.h>
  #define FFS32_HAS_BitScanForward
#elif defined(__GNUC__) || defined(__clang__)
  // GCC/Clang 平台
  #define FFS32_HAS__builtin_ffs
#else
  // 其他平台回退实现
  #undef FFS32_HAS_BitScanForward
  #undef FFS32_HAS__builtin_ffs
#endif

// 静态断言
#define FFS32_StaticAssert(cond, msg) \
    typedef char __FFS32_StaticAssert_##msg[(cond) ? 1 : -1]

// 断言
#define FFS32_Assert(p) assert((p) && "Assertion failed: " #p)

#ifdef __cplusplus
extern "C" {
#endif

// 确保 uint32_t 为4字节
FFS32_StaticAssert(sizeof(uint32_t) == 4, uint32_t_must_be_4_bytes);

/**
 * @brief 查找第一个置位（1-based）
 * @param flag 待查找的32位标志
 * @return 第一个置位的位置（1-based），全0返回0
 */
static inline int FFS32_first_setbit(uint32_t flag)
{
    if (flag == 0) {
        return 0;
    }
    if (flag == UINT32_MAX) {
        return 1;
    }
#if defined(FFS32_HAS_BitScanForward)
    unsigned long index;
    return _BitScanForward(&index, flag) ? (int)(index + 1) : 0;
#elif defined(FFS32_HAS__builtin_ffs)
    return __builtin_ctzl((unsigned long) flag) + 1;
#else
    // De Bruijn 序列实现
    static const int debruijn_table[32] = {
        0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
        31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
    };
    return (int) debruijn_table[(uint32_t)((flag & (~flag + 1)) * 0x077CB531U) >> 27] + 1;
#endif
}

/**
 * @brief 查找连续 n 个置位的起始位置（1-based）
 * @param flag 待查找的32位标志
 * @param n 连续置位的数量
 * @return 起始位置（1-based），未找到返回0
 */
static inline int FFS32_first_setbit_n(uint32_t flag, int n)
{
    FFS32_Assert(n > 0 && n <= 32);
    if (n == 1) {
        return FFS32_first_setbit(flag);
    }
    if (n == 32) {
        return (flag == UINT32_MAX) ? 1 : 0;
    }
    // 优化位掩码滑动窗口
    uint32_t mask = (1UL << n) - 1;
    for (int shift = 0; shift <= 32 - n; ++shift) {
        if ((flag & (mask << shift)) == (mask << shift)) {
            return shift + 1;
        }
    }
    return 0;
}

/**
 * @brief 从指定位置开始查找下一个置位（1-based）
 * @param flag 待查找的32位标志
 * @param startbit 起始位置（1-based）
 * @return 下一个置位的位置（1-based），未找到返回0
 */
static inline int FFS32_next_setbit(uint32_t flag, int startbit)
{
    FFS32_Assert(startbit > 0 && startbit <= 32);
    if (flag == 0) {
        return 0;
    }
    if (flag == UINT32_MAX) {
        return startbit;
    }
    int start0 = startbit - 1;
    uint32_t masked = flag >> start0;
    int pos = FFS32_first_setbit(masked);
    return pos ? (pos + start0) : 0;
}

/**
 * @brief 从指定位置开始查找下一个置0位（1-based）
 * @param flag 待查找的32位标志
 * @param startbit 起始位置（1-based）
 * @return 下一个置0位的位置（1-based），未找到返回0
 */
static inline int FFS32_next_unsetbit(uint32_t flag, int startbit)
{
    FFS32_Assert(startbit > 0 && startbit <= 32);
    if (flag == 0) {
        return startbit;
    }
    if (flag == UINT32_MAX) {
        return 0;
    }
    int start0 = startbit - 1;
    uint32_t inverted = ~flag;
    uint32_t masked = inverted >> start0;
    int pos = FFS32_first_setbit(masked);
    return pos ? (pos + start0) : 0;
}


/**
 * @brief 计算32位整数中置位的个数
 * @param flag 待检查的32位标志
 * @return 置位的个数: [0,32]
 */
static inline uint32_t FFS32_setbit_popcount(uint32_t flag)
{
    if (flag == 0) {
        return 0;
    }
    if (flag == UINT32_MAX) {
        return 32;
    }
#if defined(__POPCNT__) || (defined(__GNUC__) && defined(__SSE4_2__))
    return __builtin_popcount(flag);
#elif defined(_MSC_VER)
    return __popcnt(flag);
#else
    flag = flag - ((flag >> 1) & 0x55555555);
    flag = (flag & 0x33333333) + ((flag >> 2) & 0x33333333);
    flag = (flag + (flag >> 4)) & 0x0F0F0F0F;
    return (flag * 0x01010101) >> 24;
#endif
}

#ifdef __cplusplus
}
#endif
#endif /* FFS32_H_ */