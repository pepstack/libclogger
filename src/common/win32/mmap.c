/**
 * mman.c
 *  mmap api for windows only.
 *
 *  In particular, mmap, munmap, and msync are not available on Windows. These
 *   replacements are used on both Cygwin and MinGW. (On Cygwin the built-in mmap
 *   has no write support, and is not used).
 *
 * Special thanks to Horea Haitonic for contributing mmap.c and mman.h
 * Original Author is Horea Haitonic, April 2007
 * Updated by ZhangLiang to support 64bits, 2015
 */
#include <stdlib.h>

#include <Windows.h>

#ifdef _WIN32
# include <io.h>
#endif

#include <errno.h>

#include "mman.h"


/**
 * @brief Map a file to a memory region
 *
 * This function emulates the POSIX mmap() using CreateFileMapping() and
 * MapViewOfFile()
 *
 * @param addr the suggested start address (if != 0)
 * @param len length of the region
 * @param prot region accesibility, bitwise OR of PROT_READ, PROT_WRITE, PROT_EXEC
 * @param flags mapping type and options (ignored)
 * @param fd object to be mapped into memory
 * @param offset offset into mapped object
 * @return pointer to the memory region, or NULL in case of error
 */
void *mmap(void *addr, size_t len, int prot, int flags, int fd, unsigned int offset)
{
	DWORD wprot;
	DWORD waccess;
	HANDLE h;
	void *region;

	/* Translate read/write/exec flags into WIN32 constants */
	switch (prot) {
	case PROT_READ:
		wprot = PAGE_READONLY;
		break;
	case PROT_EXEC:
		wprot = PAGE_EXECUTE_READ;
		break;
	case PROT_READ | PROT_EXEC:
		wprot = PAGE_EXECUTE_READ;
		break;
	case PROT_WRITE:
		wprot = PAGE_READWRITE;
		break;
	case PROT_READ | PROT_WRITE:
		wprot = PAGE_READWRITE;
		break;
	case PROT_READ | PROT_WRITE | PROT_EXEC:
		wprot = PAGE_EXECUTE_READWRITE;
		break;
	case PROT_WRITE | PROT_EXEC:
		wprot = PAGE_EXECUTE_READWRITE;
		break;
	}

	/* Obtaing handle to map region */
	h = CreateFileMappingA((HANDLE) _get_osfhandle(fd), 0, wprot, HIDWORD(len), LODWORD(len), 0);
	if (h == NULL) {
		DWORD error = GetLastError();

		/* Try and translate some error codes */
		switch (error) {
		case ERROR_ACCESS_DENIED:
		case ERROR_INVALID_ACCESS:
			errno = EACCES;
			break;
		case ERROR_OUTOFMEMORY:
		case ERROR_NOT_ENOUGH_MEMORY:
			errno = ENOMEM;
			break;
		default:
			errno = EINVAL;
			break;
		}
		return MAP_FAILED;
	}


	/* Translate sharing options into WIN32 constants */
	switch (wprot) {
	case PAGE_READONLY:
		waccess = FILE_MAP_READ;
		break;
	case PAGE_READWRITE:
		waccess = FILE_MAP_WRITE;
		break;
	}

	/* Map file and return pointer */
	region = MapViewOfFile(h, waccess, 0, 0, 0);
	if (region == NULL) {
		DWORD error = GetLastError();

		/* Try and translate some error codes */
		switch (error) {
		case ERROR_ACCESS_DENIED:
		case ERROR_INVALID_ACCESS:
			errno = EACCES;
			break;
		case ERROR_INVALID_HANDLE:
			errno = EBADF;
			break;
		default:
			errno = EINVAL;
			break;
		}
		CloseHandle(h);
		return MAP_FAILED;
	}

    /* OK to call UnmapViewOfFile after this */
	CloseHandle(h);

	/* All fine */
	return region;
}


/**
 * @brief Unmap a memory region
 *
 * This is a wrapper around UnmapViewOfFile in the win32 API
 *
 * @param addr start address
 * @param len length of the region
 * @return 0 for success, -1 for error
 */
int munmap(void *addr, size_t len)
{
	if (UnmapViewOfFile(addr)) {
		return 0;
	}
	else {
		errno = EINVAL;
		return -1;
	}
}


/**
 * Synchronize a mapped region
 *
 * This is a wrapper around FlushViewOfFile
 *
 * @param addr start address
 * @param len number of bytes to flush
 * @param flags sync options -- currently ignored
 * @return 0 for success, -1 for error
 */
int msync(char *addr, size_t len, int flags)
{
	if (FlushViewOfFile(addr, len) == 0) {
		DWORD error = GetLastError();

		/* Try and translate some error codes */
		switch (error) {
		case ERROR_INVALID_PARAMETER:
			errno = EINVAL;
			break;
		case ERROR_WRITE_FAULT:
			errno = EIO;
			break;
		default:
			errno = EINVAL;
			break;
		}
		return -1;
	}

	/* Success */
	return 0;
}
