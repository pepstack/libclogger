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
** @date 2025-03-20 16:04:00
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

#define FFS32_BITS   32
#define FFS32_MAX    UINT32_MAX

typedef uint32_t FFS32_t;

// numb 对齐到 M，M 必须是 2, 4, 8, 16, ...
#define FFS32_ALIGN_UP(numb, M)  (((FFS32_t)(numb) + (FFS32_t)(M) - 1) & ~((FFS32_t)(M)-1))


// 静态断言
#define FFS32_StaticAssert(cond, msg) \
    typedef char __FFS32_StaticAssert_##msg[(cond) ? 1 : -1]

// 断言
#define FFS32_Assert(p) assert((p) && "FFS32 Assertion failed: " #p)

#ifdef __cplusplus
extern "C" {
#endif

// 确保 FFS32_t 为4字节
FFS32_StaticAssert(sizeof(FFS32_t)*8 == FFS32_BITS, FFS32_must_be_32_bits);

// 预定义掩码表：bitCount=0~32 对应的掩码: 连续 bitCount 个1
// 掩码表数值无需考虑字节序
static const FFS32_t FFS32_masks_table[FFS32_BITS + 1] = { 0x00000000,
    0x00000001, 0x00000003, 0x00000007, 0x0000000F,
    0x0000001F, 0x0000003F, 0x0000007F, 0x000000FF,
    0x000001FF, 0x000003FF, 0x000007FF, 0x00000FFF,
    0x00001FFF, 0x00003FFF, 0x00007FFF, 0x0000FFFF,
    0x0001FFFF, 0x0003FFFF, 0x0007FFFF, 0x000FFFFF,
    0x001FFFFF, 0x003FFFFF, 0x007FFFFF, 0x00FFFFFF,
    0x01FFFFFF, 0x03FFFFFF, 0x07FFFFFF, 0x0FFFFFFF,
    0x1FFFFFFF, 0x3FFFFFFF, 0x7FFFFFFF, 0xFFFFFFFF
};

#define FFS32_LeftMask(offset, start)  ((FFS32_t)(FFS32_masks_table[(offset)] << (start)))

/**
 * @brief 查找第一个置位（1-based）
 * @param flag 待查找的32位标志
 * @return 第一个置位的位置（1-based），全0返回0
 */
static inline int FFS32_first_setbit(FFS32_t flag)
{
    if (flag == 0) {
        return 0;
    }
    if (flag == FFS32_MAX) {
        return 1;
    }
#if defined(FFS32_HAS_BitScanForward)
    unsigned long index;
    return _BitScanForward(&index, flag) ? (int)(index + 1) : 0;
#elif defined(FFS32_HAS__builtin_ffs)
    return __builtin_ctzl((unsigned long) flag) + 1;
#else
    // De Bruijn 序列实现
    static const int debruijn32_table[FFS32_BITS] = {
        0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
        31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
    };
    static const FFS32_t debruijn32 = 0x077CB531U;
    return (int) debruijn32_table[(FFS32_t)((flag & (~flag + 1)) * debruijn32) >> 27] + 1;
#endif
}

/**
 * @brief 查找最后一个置位（1-based）
 * @param flag 待查找的32位标志
 * @return 反向查找第一个置位的位置（1-based），全0返回0
 */
static int FFS32_last_setbit(FFS32_t flag)
{
    if (flag == 0) {
        return 0;
    }
    if (flag == FFS32_MAX) {
        return FFS32_BITS; // 全1时最高位是32
    }

#if defined(_MSC_VER)
    // MSVC 平台优化
    unsigned long index;
    return _BitScanReverse(&index, flag) ? (int)(index + 1) : 0;

#elif defined(__GNUC__) || defined(__clang__)
    // GCC/Clang 平台优化
    return flag ? (FFS32_BITS - __builtin_clz(flag)) : 0;

#else
    // 通用位操作实现（无编译器内置函数时）
    // 原理：通过位填充找到最高有效位
    flag |= flag >> 1;
    flag |= flag >> 2;
    flag |= flag >> 4;
    flag |= flag >> 8;
    flag |= flag >> 16;
    flag = (flag >> 1) + 1; // 保留最高位

    // 使用相同的 De Bruijn 序列查找
    static const int debruijn32_table[FFS32_BITS] = {
        0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
        31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
    };
    static const FFS32_t debruijn32 = 0x077CB531U;
    return debruijn32_table[(flag * debruijn32) >> 27] + 1;
#endif
}

/**
 * @brief 查找连续 n 个置位的起始位置（1-based）
 * @param flag 待查找的32位标志
 * @param n 连续置位的数量
 * @return 起始位置（1-based），未找到返回0
 */
static inline int FFS32_first_setbit_n(FFS32_t flag, int n)
{
    FFS32_Assert(n > 0 && n <= FFS32_BITS);
    if (n == 1) {
        return FFS32_first_setbit(flag);
    }
    if (n == FFS32_BITS) {
        return (flag == FFS32_MAX) ? 1 : 0;
    }
    // 优化位掩码滑动窗口
    FFS32_t mask = (1UL << n) - 1;
    for (int shift = 0; shift <= FFS32_BITS - n; ++shift) {
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
static inline int FFS32_next_setbit(FFS32_t flag, int startbit)
{
    FFS32_Assert(startbit > 0 && startbit <= FFS32_BITS);
    if (flag == 0) {
        return 0;
    }
    if (flag == FFS32_MAX) {
        return startbit; // 1-based
    }
    int start0 = startbit - 1;
    FFS32_t masked = flag >> start0;
    int pos = FFS32_first_setbit(masked);
    return pos ? (pos + start0) : 0;
}

/**
 * @brief 从指定位置开始查找下一个置0位（1-based）
 * @param flag 待查找的32位标志
 * @param startbit 起始位置（1-based）
 * @return 下一个置0位的位置（1-based），未找到返回0
 */
static inline int FFS32_next_unsetbit(FFS32_t flag, int startbit)
{
    FFS32_Assert(startbit > 0 && startbit <= FFS32_BITS);
    if (flag == 0) {
        return startbit;
    }
    if (flag == FFS32_MAX) {
        return 0;
    }
    int start0 = startbit - 1;
    FFS32_t inverted = ~flag;
    FFS32_t masked = inverted >> start0;
    int pos = FFS32_first_setbit(masked);
    return pos ? (pos + start0) : 0;
}


/**
 * @brief 计算32位整数中置位的个数
 * @param flag 待检查的32位标志
 * @return 置位的个数: [0,32]
 */
static inline int FFS32_setbit_popcount(FFS32_t flag)
{
    if (flag == 0) {
        return 0;
    }
    if (flag == FFS32_MAX) {
        return FFS32_BITS;
    }
#if defined(__POPCNT__) || (defined(__GNUC__) && defined(__SSE4_2__))
    return (int)__builtin_popcount(flag);
#elif defined(_MSC_VER)
    return (int)__popcnt(flag);
#else
    flag = flag - ((flag >> 1) & 0x55555555);
    flag = (flag & 0x33333333) + ((flag >> 2) & 0x33333333);
    flag = (flag + (flag >> 4)) & 0x0F0F0F0F;
    return (int)((flag * 0x01010101) >> 24);
#endif
}

/**
 * @brief 在FFS32标志数组中查找连续置位数为 bitsCount 的位置
 *
 * 此函数从指定的起始 flag 开始，在FFS32标志数组中查找足够数量的连续置位（值=1的位），
 * 若找到则返回其在 flag 中的偏移（1-based），并更新起始指针到新 flag 位置。
 *
 * @param ppFlagStart   [in/out] 指向标志数组的起始指针。函数成功时更新为指向找到位置的 flag 的指针。
 * @param pFlagEndStop  [in]     标志数组的结束边界指针（不包含在搜索范围内）。
 * @param bitsCount     [in]     需要设置的连续置位数。
 *
 * @return int
 *   - 成功 > 0: 找到的偏移值，是在更新为找到位置的 flag 中的偏移（1-based）。
 *   - 失败: 返回 0（可用空间不足或参数无效）。
 *
 * flags=[flag1|flag2|flag3] => [0b1111000 | 0b11111111 | 0b00000111]，连续块数=4+8+3=15
 */
static int FFS32_flags_setbits(FFS32_t** ppFlagStart, const FFS32_t* pFlagEndStop, const int bitsCount)
{
    FFS32_t* pFirstFlag = NULL;
    int firstBitOffset = 0;

    int startBit = 1;
    int remaining = bitsCount;

    FFS32_t* pFlag = *ppFlagStart;
    while (pFlag < pFlagEndStop && remaining > 0) {
        FFS32_Assert(startBit > 0);  // startBit: 1-based

        startBit = FFS32_next_setbit(*pFlag, startBit);
        if (!startBit) { // 无置位
            remaining = bitsCount;  // 重置
            pFirstFlag = NULL;
            ++pFlag;
            startBit = 1;
            continue;
        }

        if (pFirstFlag) {
            // 已经设置起始，startBit 必须从 1 开始
            if (startBit != 1) {
                // 出现清零位，重置（pFlag 不能移动）
                remaining = bitsCount;
                pFirstFlag = pFlag;
                firstBitOffset = startBit;
            }
        }
        else { // 仅在此处设置起始
            if (bitsCount == 1) {
                // 只取1位，成功返回。此处是优化关键
                *ppFlagStart = pFlag;
                return startBit;
            }
            // 多于1位，要继续判断
            remaining = bitsCount;
            pFirstFlag = pFlag;
            firstBitOffset = startBit;
        }

        FFS32_Assert(pFirstFlag && startBit);

        // endBit=0 表示从第 startBit 位（0-based）开始到结束没有清0位。
        int endBit = FFS32_next_unsetbit(*pFlag, startBit + 1);
        int available = endBit ? endBit - startBit : FFS32_BITS - startBit + 1;

        if (remaining <= available) {
            // 发现可用的连续位，成功仅在此处返回
            *ppFlagStart = pFirstFlag;
            return firstBitOffset;
        }

        // 不满足连续置位数: remaining > available
        if (endBit) { // 出现清零位，重置（pFlag 不能移动）
            remaining = bitsCount;
            pFirstFlag = NULL;
            startBit = endBit;  // 防止无限循环
        }
        else {
            FFS32_Assert(endBit == 0);  // 不存在清零位
            remaining -= available;
            startBit = 1;
            ++pFlag; // 下一个 Flag
        }
    }

    // 未发现可用的连续位(bitsCount)
    return 0;
}


#ifdef __cplusplus
}
#endif
#endif /* FFS32_H_ */