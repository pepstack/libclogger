/*
** @file memalign.c
**   A pair of cross - platform C functions to allocate and free aligned memory
** @author Liang Zhang <350137278@qq.com>
** @version 0.0.1
** @since 2025-03-22 12:54:00
** @date
** @see
**   https://github.com/NickStrupat/AlignedMalloc.git
*/
#include "memalign.h"

bool memalign_is_aligned(const void* pointer, size_t alignment)
{
	if (!pointer) {
		return false; // 无效输入
	}

	if (!alignment) {
		alignment = memalign_alignment(0);
	}

	if (alignment % sizeof(void*) == 0 && (alignment & (alignment - 1)) == 0) {
		uintptr_t address = (uintptr_t)pointer;
		uintptr_t mask = alignment - 1;
		return (address & mask) == 0;
	}

	return false; // 无效输入
}


#if defined(__APPLE__) || defined(__linux__)

#include <stdlib.h>

void* memalign_alloc(size_t size, size_t alignment)
{
	MEM_ALIGN_ASSERT(alignment);
	void* pointer;
	int err = posix_memalign(&pointer, alignment, size);
	return err == 0? pointer : NULL;
}

void memalign_free(void* pointer)
{
	free(pointer);
}

void memalign_free_safe(void** pPointer)
{
	if (pPointer) {
		void* pointer = *pPointer;
		if (pointer) {
			*pPointer = 0;
			free(pointer);
		}
	}
}

#if defined(__APPLE__)
#include <sys/sysctl.h>

size_t memalign_alignment(size_t defaultOnFail)
{
	int mib[2];
	size_t cacheLineSize = 0;
	size_t size = sizeof(cacheLineSize);

	mib[0] = CTL_HW;
	mib[1] = HW_CACHELINE;

	// 通过 MIB 查询
	int ret = sysctl(mib, 2, &cacheLineSize, &size, NULL, 0);
	if (ret == 0) {
		return cacheLineSize;
	}
	else {
		return defaultOnFail ? defaultOnFail : MEM_ALIGN_SIZE_64;
	}
}

#else // __linux__

size_t memalign_alignment(size_t defaultOnFail)
{
	unsigned lineSize = 0;
	FILE* fp = fopen("/sys/devices/system/cpu/cpu0/cache/index0/coherency_line_size", "r");
	if (fp) {
		if (fscanf(fp, "%u", &lineSize) != 1) {
			lineSize = 0;
		}
		fclose(fp);
	}
	return (size_t)(lineSize ? lineSize : (defaultOnFail ? defaultOnFail : MEM_ALIGN_SIZE_64));
}

#endif

#elif defined(_WIN32) || defined(__WINDOWS__)
#include <Windows.h>
#include <crtdbg.h>
#include <malloc.h>

void* memalign_alloc(size_t size, size_t alignment)
{
	MEM_ALIGN_ASSERT(alignment);
#ifdef _DEBUG
	// This is reduced to a call to `_aligned_malloc` when _DEBUG is not defined
	return _aligned_malloc_dbg(size, alignment, __FILE__, __LINE__);
#else
	return _aligned_malloc(size, alignment);
#endif
}

void memalign_free(void* pointer)
{
#ifdef _DEBUG
	// This is reduced to a call to `_aligned_free` when _DEBUG is not defined
	_aligned_free_dbg(pointer);
#else
	_aligned_free(pointer);
#endif
}

void memalign_free_safe(void** pPointer)
{
	if (pPointer) {
		void* pointer = *pPointer;
		if (pointer) {
			*pPointer = 0;
			memalign_free(pointer);
		}
	}
}

size_t memalign_alignment(size_t defaultOnFail)
{
	DWORD bufferSize = 0;
	if (!GetLogicalProcessorInformation(0, &bufferSize) && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
		return defaultOnFail? defaultOnFail : MEM_ALIGN_SIZE_64;
	}

	SYSTEM_LOGICAL_PROCESSOR_INFORMATION* buffer = (SYSTEM_LOGICAL_PROCESSOR_INFORMATION*) memalign_alloc(bufferSize, MEM_ALIGN_SIZE_64);
	if (!GetLogicalProcessorInformation(buffer, &bufferSize)) {
		memalign_free_safe((void**) &buffer);
		return defaultOnFail ? defaultOnFail : MEM_ALIGN_SIZE_64;
	}

	DWORD cacheLineSize = 0;
	for (DWORD i = 0; i < bufferSize / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION); i++) {
		if (buffer[i].Relationship == RelationCache && buffer[i].Cache.Level == 1) {
			cacheLineSize = buffer[i].Cache.LineSize;
			break;
		}
	}

	memalign_free_safe((void**)&buffer);
	return (size_t) cacheLineSize ? cacheLineSize : (defaultOnFail ? defaultOnFail : MEM_ALIGN_SIZE_64);
}

#else

// https://sites.google.com/site/ruslancray/lab/bookshelf/interview/ci/low-level/write-an-aligned-malloc-free-function
#include <stdlib.h>

size_t memalign_alignment(size_t defaultOnFail)
{
	return defaultOnFail? defaultOnFail : MEM_ALIGN_SIZE_64;
}

void* memalign_alloc(size_t size, size_t alignment)
{
	MEM_ALIGN_ASSERT(alignment);
	void* p1;
	void** p2;
	const size_t offset = alignment; // 关键修正点
	if ((p1 = malloc(size + offset)) == NULL) {
		return NULL;
	}
	p2 = (void**)(((uintptr_t)p1 + offset) & ~(alignment - 1));

	// 安全屏障：确保 p2 前方有足够空间
	if ((uintptr_t)p2 - (uintptr_t)p1 < sizeof(void*)) {
		p2 = (void**)((uintptr_t)p2 + alignment);
	}
	p2[-1] = p1; // 安全写入
	return p2;
}

void memalign_free(void* pointer)
{
	free(((void**)pointer)[-1]);
}

void memalign_free_safe(void** pPointer)
{
	if (pPointer) {
		void* pointer = *pPointer;
		if (pointer) {
			*pPointer = 0;
			memalign_free(pointer);
		}
	}
}

#endif
