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
#include <stdbool.h>

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

#define FFS64_BITS   64
#define FFS64_MAX    UINT64_MAX

typedef uint64_t FFS64_t;

// numb 对齐到 M，M 必须是 2, 4, 8, 16, ...
#define FFS64_ALIGN_UP(numb, M)    (((FFS64_t)(numb) + (FFS64_t)(M) - 1) & ~((FFS64_t)(M)-1))


// 静态断言
#define FFS64_StaticAssert(cond, msg) \
    typedef char __FFS64_StaticAssert_##msg[(cond) ? 1 : -1]

// 断言
#define FFS64_Assert(p) assert((p) && "FFS64 Assertion failed: " #p)

#ifdef __cplusplus
extern "C" {
#endif

// 确保 FFS64_t 为 8 字节
FFS64_StaticAssert(sizeof(FFS64_t)*8 == FFS64_BITS, FFS64_must_be_64_bits);


// 预定义掩码表：bitCount=0~64 对应的掩码: 连续 bitCount 个1（低 bitCount 位为1）
// 掩码表数值无需考虑字节序
static const FFS64_t FFS64_masks_table[FFS64_BITS + 1] = { 0x0000000000000000,
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

#define FFS64_LeftMask(offset, start)  ((FFS64_t)(FFS64_masks_table[(offset)] << (start)))


/**
 * @brief 查找第一个置位（1-based）
 * @param flag 待查找的64位标志
 * @return 第一个置位的位置（1-based），全0返回0
 */
static inline int FFS64_first_setbit(FFS64_t flag)
{
    if (flag == 0) {
        return 0;
    }
    if (flag == FFS64_MAX) {
        return 1;
    }
#if defined(FFS64_HAS_BitScanForward)
    unsigned long index;
    return _BitScanForward64(&index, flag) ? (int)(index + 1) : 0;
#elif defined(FFS64_HAS__builtin_ffs)
    return __builtin_ffsll(flag);
#else
    // De Bruijn 序列实现
    static const int debruijn64_table[FFS64_BITS] = {
        0,  1, 48,  2, 57, 49, 28,  3,
        61, 58, 50, 42, 38, 29, 17,  4,
        62, 55, 59, 36, 53, 51, 43, 22,
        45, 39, 33, 30, 24, 18, 12,  5,
        63, 47, 56, 27, 60, 41, 37, 16,
        54, 35, 52, 21, 44, 32, 23, 11,
        46, 26, 40, 15, 34, 20, 31, 10,
        25, 14, 19,  9, 13,  8,  7,  6
    };
    static const FFS64_t debruijn64 = 0x03F79D71B4CB0A89ULL;
    return debruijn64_table[((flag & (~flag + 1)) * debruijn64) >> 58] + 1;
#endif
}

/**
 * @brief 查找最后一个置位（1-based）
 * @param flag 待查找的64位标志
 * @return 最后一个置位的位置（1-based），全0返回0
 */
static int FFS64_last_setbit(FFS64_t flag)
{
    if (flag == 0) {
        return 0;
    }
    if (flag == FFS64_MAX) {
        return FFS64_BITS; // 全1时最高位是64
    }
#if defined(_MSC_VER)
    // MSVC 平台优化（需支持64位指令集）
    unsigned long index;
    return _BitScanReverse64(&index, flag) ? (int)(index + 1) : 0;

#elif defined(__GNUC__) || defined(__clang__)
    // GCC/Clang 平台优化
    return flag ? (FFS64_BITS - __builtin_clzll(flag)) : 0;
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
    static const int debruijn64_table[FFS64_BITS] = {
        0,  1, 48,  2, 57, 49, 28,  3,
        61, 58, 50, 42, 38, 29, 17,  4,
        62, 55, 59, 36, 53, 51, 43, 22,
        45, 39, 33, 30, 24, 18, 12,  5,
        63, 47, 56, 27, 60, 41, 37, 16,
        54, 35, 52, 21, 44, 32, 23, 11,
        46, 26, 40, 15, 34, 20, 31, 10,
        25, 14, 19,  9, 13,  8,  7,  6
    };
    static const FFS64_t debruijn64 = 0x03F79D71B4CB0A89ULL;
    return debruijn64_table[(flag * debruijn64) >> 58] + 1;
#endif
}

/**
 * @brief 查找连续 n 个置位的起始位置（1-based）
 * @param flag 待查找的64位标志
 * @param n 连续置位的数量
 * @return 起始位置（1-based），未找到返回0
 */
static inline int FFS64_first_setbit_n(FFS64_t flag, int n)
{
    FFS64_Assert(n > 0 && n <= FFS64_BITS);
    if (n == 1) {
        return FFS64_first_setbit(flag);
    }
    if (n == FFS64_BITS) {
        return (flag == FFS64_MAX) ? 1 : 0;
    }
    const FFS64_t mask = (1ULL << n) - 1;
    for (int shift = 0; shift <= FFS64_BITS - n; ++shift) {
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
static inline int FFS64_next_setbit(FFS64_t flag, int startbit)
{
    FFS64_Assert(startbit > 0 && startbit <= FFS64_BITS);
    if (flag == 0) {
        return 0;
    }
    if (flag == FFS64_MAX) {
        return startbit; // 1-based
    }
    const int start0 = startbit - 1;
    const FFS64_t masked = flag >> start0;
    const int pos = FFS64_first_setbit(masked);
    return pos ? (pos + start0) : 0;
}

/**
 * @brief 从指定位置开始查找下一个置0位（1-based）
 * @param flag 待查找的64位标志
 * @param startbit 起始位置（1-based）
 * @return 下一个置0位的位置（1-based），未找到返回0
 */
static inline int FFS64_next_unsetbit(FFS64_t flag, int startbit)
{
    FFS64_Assert(startbit > 0 && startbit <= FFS64_BITS);
    if (flag == FFS64_MAX) {
        return 0;
    }
    const int start0 = startbit - 1;
    const FFS64_t inverted = ~flag;
    const FFS64_t masked = inverted >> start0;
    const int pos = FFS64_first_setbit(masked);
    return pos ? (pos + start0) : 0;
}

/**
 * @brief 计算64位整数中置位的个数
 * @param flag 待检查的64位标志
 * @return 置位的个数: [0,64]
 */
static inline int FFS64_setbit_popcount(FFS64_t flag)
{
    if (flag == 0) {
        return 0;
    }
    if (flag == FFS64_MAX) {
        return FFS64_BITS;
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

/**
 * @brief 在FFS64标志数组中查找连续置位数为 bitsCount 的位置
 *
 * 此函数从指定的起始 flag 开始，在FFS64标志数组中查找足够数量的连续置位（值=1的位），
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
static int FFS64_flags_find_setbits(FFS64_t** ppFlagStart, const FFS64_t* pFlagEndStop, const int bitsCount)
{
    FFS64_t* pFirstFlag = NULL;
    int firstBitOffset = 0;

    int startBit = 1;
    int remaining = bitsCount;

    FFS64_t* pFlag = *ppFlagStart;
    while (pFlag < pFlagEndStop && remaining > 0) {
        FFS64_Assert(startBit > 0);  // startBit: 1-based

        startBit = FFS64_next_setbit(*pFlag, startBit);
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

        FFS64_Assert(pFirstFlag && startBit);

        // endBit=0 表示从第 startBit 位（0-based）开始到结束没有清0位。
        int endBit = FFS64_next_unsetbit(*pFlag, startBit + 1);
        int available = endBit ? endBit - startBit : FFS64_BITS - startBit + 1;

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
            FFS64_Assert(endBit == 0);  // 不存在清零位
            remaining -= available;
            startBit = 1;
            ++pFlag; // 下一个 Flag
        }
    }

    // 未发现可用的连续位(bitsCount)
    return 0;
}


static void FFS64_flags_mask_bits(FFS64_t* pFlag, int bitOffset, int bitCount, bool is_set_1)
{
    FFS64_t mask;
    int startBit = bitOffset;

    while (bitCount > 0) {
        // 当前 flag 需要 mask 位数
        bitOffset = FFS64_BITS - startBit;
        if (bitOffset > bitCount) {
            bitOffset = bitCount;
        }

        if (is_set_1) { // 生成 set 掩码：将 bitOffset 开始的 bitCount 个位设为 1
            mask = FFS64_LeftMask(bitOffset, startBit);
            *pFlag |= mask; // 置位=1
        }
        else {          // 生成 unset 掩码：将 bitOffset 开始的 bitCount 个位设为 0
            mask = ~FFS64_LeftMask(bitOffset, startBit);
            *pFlag &= mask; // 清除=0
        }

        bitCount -= bitOffset;
        if (bitCount == 0) {
            break;  // 成功设置
        }

        startBit = 0;
        ++pFlag;
    }
}

#ifdef __cplusplus
}
#endif
#endif /* FFS64_H_ */