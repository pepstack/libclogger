/*
** @file memalign.h
**   A pair of cross - platform C functions to allocate and free aligned memory
** @author Liang Zhang <350137278@qq.com>
** @version 0.0.1
** @since 2025-03-22 12:54:00
** @date
** @see
**   https://github.com/NickStrupat/AlignedMalloc.git
*/
/*
## AlignedMalloc
A pair of cross - platform C functions to allocate and free aligned memory

void* aligned_malloc(size_t size, size_t alignment);
void aligned_free(void* pointer);

### Usage

Use the two functions in place of `malloc` and `free` when you need aligned memory.

### Supported platforms

Mac OS X and Linux are supported by calling `posix_memalign`,
Windows by calling `_aligned_malloc` (`_aligned_malloc_dbg` while compiling with `_DEBUG`).
Outside of those platforms, determined with the `__APPLE__`, `__linux__`, and `_WIN32`
macro definitions respectively, the function will use a custom implementation.

This custom implementation uses `malloc` under the hood, but returns a pointer to an aligned
memory address within a larger memory block.It stores the pointer returned by malloc in the
extra space before the aligned memory for `aligned_free` to access later.
This is necessary to ensure the memory can be freed correctly.
*/
#ifndef MEM_ALIGN_H_INCLUDED
#define MEM_ALIGN_H_INCLUDED

#include <stddef.h>
#include <assert.h>
#include <stdbool.h>   // C99
#include <stdint.h>    // C99

#ifdef __cplusplus
extern "C" {
#endif

// 默认对齐 64 字节。大多数 OS 支持。
#define MEM_ALIGN_SIZE_64     64U

// 检查 alignment 是否合法
#define MEM_ALIGN_ASSERT(alignment) \
	assert(((((alignment) != 0) && \
		((alignment) % sizeof(void*) == 0) && \
		((alignment) & ((alignment) - 1)) == 0)) && "memalign_assert failed: " #alignment)

// 向上对齐
#define MEM_ALIGN_UP(size, M)    (((size_t)(size) + (size_t)(M) - 1) & ~((size_t)(M)-1))

/**
 * @brief 分配对齐的内存块
 * @param size 需要分配的内存大小（字节）
 * @param alignment 对齐要求（必须是2的幂且是sizeof(void*)的倍数）
 * @return 成功返回对齐的内存指针，失败返回NULL
 * @note Windows平台自动关联调试信息（仅在_DEBUG模式下）
 */
extern void* memalign_alloc(size_t size, size_t alignment);

/**
 * @brief 释放对齐的内存块
 * @param pointer 由memalign_alloc分配的指针
 * @note 必须使用此函数释放对齐内存块（与分配函数平台匹配）
 */
extern void memalign_free(void* pointer);

/**
 * @brief 安全释放对齐内存块（自动置空指针）
 * @param pPointer 指向内存指针的二级指针
 * @warning 传入二级指针将自动置空原始指针（防御野指针）
 */
extern void memalign_free_safe(void** pPointer);

/**
 * @brief 获取当前处理器的缓存行大小
 * @return 缓存行字节数（失败时返回默认值64）
 * @remark
 * - macOS: 通过sysctl获取HW_CACHELINE
 * - Linux: 读取CPU缓存信息文件
 * - Windows: 查询逻辑处理器信息
 */
extern size_t memalign_alignment();

/**
 * @brief 验证指针是否满足对齐要求
 * @param pointer 需要验证的内存指针
 * @param alignment 对齐值（0表示使用默认缓存行对齐）
 * @retval true 指针满足对齐要求
 * @retval false 指针未对齐或参数无效
 * @warning 当alignment=0时使用运行时检测的缓存行大小
 */
extern bool memalign_is_aligned(const void* pointer, size_t alignment);

#ifdef __cplusplus
}
#endif

#endif // MEM_ALIGN_H_INCLUDED
