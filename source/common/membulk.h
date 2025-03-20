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
** @file membulk.h
** @brief 连续内存池实现 - 高效管理固定大小内存块的分配与释放
** @author LiangZhang <350137278@qq.com>
** @version 0.1.0
** @since 2025-03-14 02:54:00
** @date 2025-03-21 14:23:00
**
** @attention
** 1. 内存池线程安全但建议使用线程本地存储
** 2. 块头占用16字节，实际可用大小需减去开销
** 3. 分配失败返回NULL需显式处理
*/
#ifndef MEMBULK_H_
#define MEMBULK_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief 内存池句柄 */
typedef struct membulk_pool_t* membulk_pool;

/** @brief 内存池统计信息 */
typedef struct {
    uint32_t bulkSizeBytes;   /**< 内存块字节数（含 16 字节的元数据） */
    uint32_t bulksMaxCount;   /**< 内存块总数量 */
} membulk_stats_t;

/**
 * @brief 创建内存池实例
 * @param bulkSizeBytes 单块字节数 (128 B ≤ size ≤ 1048576 B)
 * @param bulksCapacity 总块数 (64 ≤ count ≤ 16384)
 * @return 成功返回内存池句柄，失败返回NULL
 * @note 
 * - 最小可分配: bulkSizeBytes 字节
 * - 最大可分配: bulkSizeBytes * bulksCapacity 字节
 * - 实际可用空间需减去16字节块头
 */
membulk_pool membulk_pool_create(uint32_t bulkSizeBytes, uint32_t bulksCapacity);

/**
 * @brief 销毁内存池实例
 * @param pool 内存池句柄（允许传入NULL）
 * @post 调用后句柄自动置NULL
 */
void membulk_pool_destroy(void* pool);

/**
 * @brief 从内存池分配内存
 * @param pool 内存池句柄
 * @param size 请求字节数 (会自动对齐到bulkSizeBytes)
 * @return 成功返回内存指针，失败返回NULL
 * @warning 分配失败必须进行错误处理
 */
void* membulk_alloc(membulk_pool pool, size_t size);

/**
 * @brief 从内存池分配并清零连续内存块
 * @param pool 内存池句柄
 * @param elementsCount 需要分配的元素数量（必须>0）
 * @param elementSizeBytes 单个元素的字节大小（自动对齐到bulkSizeBytes）
 * @return 成功返回已清零的内存指针，失败返回NULL
 * @warning
 * - 当elementsCount * elementSizeBytes超过内存池容量时必然失败
 * @note
 * - 内存布局保证元素连续存储，无填充字节
 */
void* membulk_calloc(membulk_pool pool, size_t elementsCount, size_t elementSizeBytes);

/**
 * @brief 释放已分配内存
 * @param pMemory 待释放指针（必须为本池分配）
 * @return 成功返回NULL，失败返回原指针：表示用户尝试释放的指针不是本池分配的。
 * @note 例如：
 *      pMemory = membulk_free(pool, pMemory);
 *      if (! pMemory) {
 *          // 成功释放内存
 *      } else {
 *          // 错误处理
 *          printf("失败: 用户尝试释放的指针不是本池分配的。\n");
 *          ..... 
 *      }
 */
void* membulk_free(membulk_pool pool, void* pMemory);

/**
 * @brief 获取内存池统计信息
 * @param pool 内存池句柄
 * @param[out] stats 统计信息结构体（可选）
 * @return 当前空闲块数
 * @note stats 为 NULL 时仅返回空闲块数
 */
int membulk_pool_stats(membulk_pool pool, membulk_stats_t* stats);

#ifdef __cplusplus
}
#endif

#endif /* MEMBULK_H_ */
