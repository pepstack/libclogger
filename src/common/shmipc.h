/***********************************************************************
 * Copyright (c) 2008-2080, 350137278@qq.com
 *
 * ALL RIGHTS RESERVED.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 *   Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **********************************************************************/

/**
 * @filename   shmipc.h
 *   shared memory ipc for Windows and Linux
 *
 * @author     Liang Zhang <350137278@qq.com>
 * @version    1.0.0
 * @create     2020-12-05 12:46:50
 * @update     2021-05-18 18:25:08
 *
 */
#ifndef SHMIPC_H__
#define SHMIPC_H__

#if defined(__cplusplus)
extern "C"
{
#endif

#include "cstrbuf.h"

#if defined(__WINDOWS__) || defined(__CYGWIN__)
    # include <Windows.h>
# else /* LINUX */
    # include <sys/ipc.h>
    # include <sys/shm.h>
#endif

#ifndef __STDC_FORMAT_MACROS
# define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>


#ifndef NOWARNING_UNUSED
    # if defined(__GNUC__) || defined(__CYGWIN__)
        # define NOWARNING_UNUSED(x) __attribute__((unused)) x
    # else
        # define NOWARNING_UNUSED(x) x
    # endif
#endif


#if defined(__WINDOWS__) || defined(__CYGWIN__)
    typedef HANDLE  shmhandle_t;
    typedef char * shmkey_t;
#else
    typedef int  shmhandle_t;
    typedef key_t shmkey_t;
#endif

#define shmipc_invalid_handle   ((shmhandle_t)(-1))


#if defined(__WINDOWS__) || defined(__CYGWIN__)

NOWARNING_UNUSED(static)
shmkey_t shmipc_keygen (const char *filename, int pid, char *keybuf, size_t bufsz)
{
    snprintf(keybuf, bufsz, "Local\\%s.%d", filename, pid);
    keybuf[bufsz - 1] = 0;
    return keybuf;
}


NOWARNING_UNUSED(static)
shmhandle_t shmipc_create (const char *key, const void *copydata, size_t copysize, size_t maxsize, size_t offset)
{
    HANDLE hMapFile;
    PVOID pvBuf;
    int err;

    if (maxsize < copysize) {
        maxsize = copysize;
    }

    hMapFile = CreateFileMappingA(
                 INVALID_HANDLE_VALUE,    // use paging file
                 NULL,                    // default security
                 PAGE_READWRITE,          // read/write access
                 (DWORD) 0,               // maximum object size (high-order DWORD)
                 (DWORD) maxsize,         // maximum object size (low-order DWORD)
                 key);                    // name of mapping object: "Local\\yourname"

    if (! hMapFile) {
        errno = GetLastError();
        return shmipc_invalid_handle;
    }

    pvBuf = MapViewOfFile(hMapFile,       // handle to map object
                FILE_MAP_ALL_ACCESS,      // read/write permission
                (DWORD) 0,
                (DWORD) offset,
                (SIZE_T) copysize);
    if (! pvBuf) {
        err = GetLastError();
        CloseHandle(hMapFile);
        errno = err;
        return shmipc_invalid_handle;
    }

    memcpy(pvBuf, copydata, copysize);

    if (! UnmapViewOfFile(pvBuf)) {
        err = GetLastError();
        CloseHandle(hMapFile);
        errno = err;
        return shmipc_invalid_handle;
    }

    return hMapFile;
}


NOWARNING_UNUSED(static)
int shmipc_read (const char *key, void *readbuf, size_t readbytes, size_t offset)
{
    HANDLE hMapFile;
    PVOID pvBuf;
    int err;

    hMapFile = OpenFileMappingA(
                    FILE_MAP_READ,   // read access
                    FALSE,           // do not inherit the name
                    key);            // name of mapping object
    if (! hMapFile) {
        errno = GetLastError();
        return -1;
    }

    pvBuf = MapViewOfFile(hMapFile,  // handle to map object
                FILE_MAP_READ,       // read permission
                (DWORD) 0,
                (DWORD) offset,
                (SIZE_T) readbytes);
    if (! pvBuf) {
        err = GetLastError();
        CloseHandle(hMapFile);
        errno = err;
        return -1;
    }

    memcpy(readbuf, pvBuf, readbytes);

    UnmapViewOfFile(pvBuf);
    CloseHandle(hMapFile);

    return 0;
}


NOWARNING_UNUSED(static)
void shmipc_destroy (shmhandle_t *hp)
{
    HANDLE h = *hp;
    if (h != shmipc_invalid_handle) {
        *hp = shmipc_invalid_handle;
        CloseHandle(h);
    }
}


#else /* LINUX */

NOWARNING_UNUSED(static)
shmkey_t shmipc_keygen (const char *filename, int pid)
{
    int pidint = pid;
    char pidstr[20];

    int idx = snprintf(pidstr, sizeof(pidstr), "%x", pid) - 1;
    if (pidstr[idx] == '0') {
        pidstr[idx] = pidstr[0];
    }

    errno = 0;
    pidint = (int) strtol(pidstr, NULL, 16);
    if (errno) {
        perror("strtol");
    }

    /* https://man7.org/linux/man-pages/man3/ftok.3.html */
    return (shmkey_t) ftok(filename, pidint);
}


NOWARNING_UNUSED(static)
shmhandle_t shmipc_create (shmkey_t key, const void *copydata, size_t copysize, size_t maxsize, size_t offset)
{
    int shmid, err;
    void *addr;

    int flags = 0600 | IPC_CREAT | IPC_EXCL;

    if (maxsize < copysize) {
        maxsize = copysize;
    }

shm_create_always:

    shmid = shmget(key, maxsize, flags);
    if (shmid == shmipc_invalid_handle) {
        if (errno != EEXIST) {
            return (-1);
        }

        shmid = shmget(key, 0, 0);
        if (shmid == shmipc_invalid_handle) {
            return (-1);
        }

        if (shmctl(shmid, IPC_RMID, 0) == -1) {
            return (-1);
        }

        goto shm_create_always;
    }

    addr = shmat(shmid, NULL, 0);
    if (!addr) {
        err = errno;
        shmctl(shmid, IPC_RMID, 0);
        errno = err;
        return (-1);
    }

    memcpy(addr, copydata, copysize);
    shmdt(addr);
    return shmid;
}


NOWARNING_UNUSED(static)
int shmipc_read (shmkey_t key, void *readbuf, size_t readbytes, size_t offset)
{
    int shmid;
    void *addr;

    shmid = shmget(key, 0, 0);
    if (shmid == shmipc_invalid_handle) {
        return (-1);
    }

    addr = shmat(shmid, 0, 0);
    if (!addr) {
        return (-1);
    }

    memcpy(readbuf, addr + offset, readbytes);
    shmdt(addr);
    return 0;
}


NOWARNING_UNUSED(static)
void shmipc_destroy (shmhandle_t *hp)
{
    int shmid = *hp;
    if (shmid != shmipc_invalid_handle) {
        *hp = shmipc_invalid_handle;
        shmctl(shmid, IPC_RMID, 0);
    }
}

#endif


#ifdef __cplusplus
}
#endif
#endif /* SHMIPC_H__ */