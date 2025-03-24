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
** @file membuff.h
** @brief 连续内存池实现 - 高效管理固定大小内存块的分配与释放
** @author LiangZhang <350137278@qq.com>
** @version 0.1.0
** @since 2025-03-14 02:54:00
** @date 2025-03-21 14:23:00
**
** @note
** 1. 内存池中的全部内存块是预先连续分配的，根据需要取出其中的1块或若干连续几块给调用者使用。
**    内存池不进行任何越界检查，任何越界访问会造成内存池不可逆的损坏和程序崩溃。
** 2. 内存池对象 membuff_pool 是多线程安全的，但是强烈建议应用本内存池时使用线程本地存储。
** 3. 单个内存块尺寸的上限是 4096 字节（实际用户可用 4080 字节）。可分配的内存块数目上限
**    为 4096*64 个，相当于最大可分配的连续内存约为 1 GB（需减去块头16字节）。
** 4. 分配失败返回 NULL，用户必须显式处理（回退到其他方式进行内存分配）。
** 5. 内存池主要用于短期持有的小内存块（<=4080字节）的高性能、高频分配和释放，不会造成内存
**    碎片。长期持有的内存请使用 malloc 或 mi_malloc。
** 6. 大页内存池请使用 mmaphuge.h。
*/
#ifndef MEM_BUFF_H_
#define MEM_BUFF_H_

#include <stdint.h> // uint32_t
#include <stddef.h> // offsetof
#include <time.h>   // struct timespec

#ifdef __cplusplus
extern "C" {
#endif

// 单个内存块大小限制 4096 B
#define MEMBUFF_BSIZE_MAX       4096U

/** @brief 内存池句柄 */
typedef struct membuff_pool_t* membuff_pool;

/** @brief 内存池统计信息
 *   内存池总大小 = buffSizeBytes * buffsMaxCount
 */
typedef struct
{
    size_t buffSizeBytes;         // 内存块字节数（含 16 字节的元数据）
    size_t buffsMaxCount;         // 内存块总数量
    struct timespec timestamp;    // 当前时间
} membuff_stat_t;

/**
 * @brief 创建内存池实例
 * @param buffSizeBytes 单块字节数 (必须对齐 128 B，且 ≤ MEMBUFF_BSIZE_MAX)
 * @param buffsCount 总块数 (64 ≤ count ≤ MEMBUFF_FLAGS_MAX)
 * @return 成功返回内存池句柄，失败返回NULL
 * @note 
 * - 最小可分配: buffSizeBytes 字节
 * - 最大可分配: buffSizeBytes * buffsCount 字节
 * - 实际可用空间需减去16字节块头
 */
extern membuff_pool membuff_pool_create(size_t buffSizeBytes, size_t buffsCount);

/**
 * @brief 销毁内存池实例
 * @param pool 内存池句柄
 * @post 调用后句柄自动置NULL
 */
extern void membuff_pool_destroy(void* pool);

/**
 * @brief 从内存池分配内存
 * @param pool 内存池句柄
 * @param size 请求字节数 (会自动对齐到buffSizeBytes)
 * @return 成功返回内存指针，失败返回NULL
 * @warning 分配失败必须进行错误处理
 */
extern void* membuff_alloc(membuff_pool pool, size_t size);

/**
 * @brief 从内存池分配并清零连续内存块
 * @param pool 内存池句柄
 * @param elementsCount 需要分配的元素数量（必须>0）
 * @param elementSizeBytes 单个元素的字节大小（自动对齐到buffSizeBytes）
 * @return 成功返回已清零的内存指针，失败返回NULL
 * @warning
 * - 当elementsCount * elementSizeBytes超过内存池容量时必然失败
 * @note
 * - 内存布局保证元素连续存储，无填充字节
 */
extern void* membuff_calloc(membuff_pool pool, size_t elementsCount, size_t elementSizeBytes);

/**
 * @brief 释放已分配内存
 * @param pool 内存池句柄
 * @param pMemory 待释放指针（必须为本池分配）
 * @return 成功返回NULL，失败返回原指针：表示用户尝试释放的指针不是本池分配的。
 * @note 例如：
 *      pMemory = membuff_free(pool, pMemory);
 *      if (! pMemory) {
 *          // 成功释放内存
 *      } else {
 *          // 错误处理
 *          printf("失败: 用户尝试释放的指针不是本池分配的。\n");
 *          ..... 
 *      }
 */
extern void* membuff_free(membuff_pool pool, void* pBuffer);

/**
 * @brief 获取内存池统计信息
 * @param pool 内存池句柄。
 * @param[out] stats 统计信息结构体（可选）
 * @return 当前空闲块数
 * @note stat 为 NULL 时仅返回空闲块数
 */
extern size_t membuff_pool_stat(membuff_pool pool, membuff_stat_t* stat);

#ifdef __cplusplus
}
#endif

#endif /* MEM_BUFF_H_ */
