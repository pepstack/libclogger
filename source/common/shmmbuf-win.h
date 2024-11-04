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
***********************************************************************/
/*
** @file      shmmbuf-win.h
**   MP and MT-safe shared memory IO for Windows and Cygwin
**
** @author     Liang Zhang <350137278@qq.com>
** @version 1.0.1
** @since     2020-06-01 12:46:50
** @date      2024-11-04 03:34:05
**
** @note
**   Prior to include this file, define as following to enable
**   trace print in spite of speed penalty.
**
** #define SHMMBUF_TRACE_PRINT_ON
** #include "shmmbuf-win.h"
*/
#ifndef SHMMBUF_WIN_H__
#define SHMMBUF_WIN_H__

#if defined(__cplusplus)
extern "C"
{
#endif

#include <Windows.h>

#ifndef __STDC_FORMAT_MACROS
# define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>


/**
 * random generator for cryptography
 */
#include "randctx.h"
#include "md5sum.h"


#ifndef NOWARNING_UNUSED
    # if defined(__GNUC__) || defined(__CYGWIN__)
        # define NOWARNING_UNUSED(x) __attribute__((unused)) x
    # else
        # define NOWARNING_UNUSED(x) x
    # endif
#endif


/**
 * Default constants only show you the usage for shmmap api.
 *
 * Shared memory file lies in: "C:\TEMP\clogger\shmmap-buffer-default"
 */
#define SHMMBUF_FILENAME_DEFAULT        "C:\\TEMP\\clogger\\shmmap-buffer-default"

#define SHMMBUF_FILEMODE_DEFAULT        0666

#define SHMMBUF_SEMAPHORE_MAXVAL        65535

/**
 * Below definitions SHOULD NOT be changed!
 */
#define SHMMBUF_PAGE_SIZE        ((size_t)4096)

#define SHMMBUF_INVALID_STATE    ((size_t)(-1))

#define SHMMBUF_TIMEOUT_INFINITE ((size_t)(-1))
#define SHMMBUF_TIMEOUT_NOWAIT   (0)

#define SHMMBUF_ENTRY_HDRSIZE    (sizeof(shmmbuf_entry_t))

#define SHMMBUF_ALIGN_BSIZE(bsz, alignsize)  \
            ((size_t)((((size_t)(bsz)+(alignsize)-1)/(alignsize))*(alignsize)))

#define SHMMBUF_ALIGN_PAGESIZE(fsz)    \
            SHMMBUF_ALIGN_BSIZE((fsz), SHMMBUF_PAGE_SIZE)

#define SHMMBUF_ALIGN_ENTRYSIZE(chunksz)    \
            SHMMBUF_ALIGN_BSIZE((chunksz + SHMMBUF_ENTRY_HDRSIZE), SHMMBUF_ENTRY_HDRSIZE)

#define SHMMBUF_BUFFER_HDRSIZE   \
            SHMMBUF_ALIGN_BSIZE(sizeof(shmmap_buffer_t), SHMMBUF_PAGE_SIZE)

#define SHMMBUF_ENTRY_CAST(p)           ((shmmbuf_entry_t *)(p))

#define SHMMBUF_VERIFY_STATE(val)    do { \
        if ((val) == SHMMBUF_INVALID_STATE) { \
            exit(EXIT_FAILURE); \
        } \
    } while(0)


/**
 * Returns of shmmap_buffer_create()
 */
#define SHMMBUF_CREATE_SUCCESS      0

#define SHMMBUF_CREATE_ERROPEN     (1)
#define SHMMBUF_CREATE_ERRMMAP     (2)
#define SHMMBUF_CREATE_ERRSIZE     (3)
#define SHMMBUF_CREATE_ERRTRUNC    (4)
#define SHMMBUF_CREATE_ERRTOKEN    (5)

/**
 * Should never run to this!
 */
#define SHMMBUF_CREATE_FAILED      (-1)

/**
 * Returns of shmmap_buffer_write()
 */
#define SHMMBUF_WRITE_SUCCESS     ((int)(1))
#define SHMMBUF_WRITE_AGAIN       ((int)(0))
#define SHMMBUF_WRITE_FATAL       ((int)(-1))


/**
 * Returns of shmmap_buffer_read_?()
 */
#define SHMMBUF_READ_NEXT         ((int)(1))
#define SHMMBUF_READ_AGAIN        ((int)(0))
#define SHMMBUF_READ_FATAL        ((int)(-1))


/**
 * default token cryptographic algorithm
 */
typedef __ub8_t ub8token_t;


NOWARNING_UNUSED(static)
ub8token_t shmmbuf_encipher_token (const ub8token_t magic, ub8token_t *token)
{
    ub8token_t cipher = magic ^ (*token);
    return cipher;
}


NOWARNING_UNUSED(static)
ub8token_t shmmbuf_decipher_token (const ub8token_t cipher, ub8token_t *token)
{
    ub8token_t magic = cipher ^ (*token);
    return magic;
}


NOWARNING_UNUSED(static)
void ProcessMutexInit (HANDLE *mutexp, const char *name)
{
    DWORD dwErr;
    HANDLE mutex;

    mutex = CreateMutexA(NULL, FALSE, name);
    if (! mutex) {
        exit(EXIT_FAILURE);
    }

    dwErr = GetLastError();
    if ( dwErr && dwErr != ERROR_ALREADY_EXISTS ) {
        CloseHandle(mutex);
        exit(EXIT_FAILURE);
    }

    *mutexp = mutex;
}


NOWARNING_UNUSED(static)
int ProcessMutexTryLock (HANDLE mutex)
{
    return (int) WaitForSingleObject(mutex, 0);
}


NOWARNING_UNUSED(static)
int ProcessMutexTimedLock (HANDLE mutex, int dwMilliSeconds)
{
    return (int) WaitForSingleObject(mutex, (DWORD) dwMilliSeconds);
}


NOWARNING_UNUSED(static)
int ProcessMutexLock (HANDLE mutex)
{
    return (int) WaitForSingleObject(mutex, INFINITE);
}


NOWARNING_UNUSED(static)
int ProcessMutexUnlock (HANDLE mutex)
{
    return ReleaseMutex(mutex)? 0 : (-1);
}


NOWARNING_UNUSED(static)
void ProcessMutexFixLockErr(HANDLE mutex, int err)
{
    // TODO:
    printf("(shmmbuf-win.h:%d) ProcessMutexFixLockErr: invalid mutex.\n", __LINE__);
    exit(EXIT_FAILURE);
}


/**
 * The layout of memory for any one entry in shmmap.
 */
typedef struct _shmmbuf_entry_t
{
    size_t size;
    char chunk[0];
} shmmbuf_entry_t;


/**
 * The atomic struct for state of shmmap.
 */
typedef struct _shmmbuf_state_t
{
    /* atomic state value */
    size_t state;

    /* process-wide state lock */
    HANDLE mutex;
} shmmbuf_state_t;


NOWARNING_UNUSED(static)
void shmmap_gettimeofday (struct timespec *now)
{
    FILETIME tmfile;
    ULARGE_INTEGER _100nanos;

    GetSystemTimeAsFileTime(&tmfile);

    _100nanos.LowPart   = tmfile.dwLowDateTime;
    _100nanos.HighPart  = tmfile.dwHighDateTime;
    _100nanos.QuadPart -= 0x19DB1DED53E8000;

    /* Convert 100ns units to seconds */
    now->tv_sec = (time_t)(_100nanos.QuadPart / (10000 * 1000));

    /* Convert remainder to nanoseconds */
    now->tv_nsec = (long) ((_100nanos.QuadPart % (10000 * 1000)) * 100);
}


NOWARNING_UNUSED(static)
sb8 shmmap_difftime_msec (const struct timespec *t1, const struct timespec *t2)
{
    sb8 sec = (sb8)(t2->tv_sec - t1->tv_sec);
    sb8 nsec = (sb8)(t2->tv_nsec - t1->tv_nsec);

    if (sec > 0) {
        if (nsec >= 0) {
            return ((sec * 1000UL) + nsec / 1000000UL);
        } else { /* nsec < 0 */
            return (sec-1) * 1000UL + (nsec + 1000000000UL) / 1000000UL;
        }
    } else if (sec < 0) {
        if (nsec <= 0) {
            return ((sec * 1000UL) + nsec / 1000000UL);
        } else { /* nsec > 0 */
            return (sec+1) * 1000UL + (nsec - 1000000000UL) / 1000000UL;
        }
    } else { /* sec = 0 */
        return nsec / 1000000UL;
    }
}


NOWARNING_UNUSED(static)
size_t shmmbuf_state_get (shmmbuf_state_t *st)
{
    size_t val = SHMMBUF_INVALID_STATE;
    int err = ProcessMutexLock(st->mutex);

    if (! err) {
        val = st->state;
        ProcessMutexUnlock(st->mutex);
        return val;
    } else {
        ProcessMutexFixLockErr(st->mutex, err);
        return shmmbuf_state_get(st);
    }

    SHMMBUF_VERIFY_STATE(val);
    return val;
}


NOWARNING_UNUSED(static)
size_t shmmbuf_state_set (shmmbuf_state_t *st, size_t newval)
{
    size_t oldval = SHMMBUF_INVALID_STATE;
    int err = ProcessMutexLock(st->mutex);

    if (! err) {
        oldval = st->state;
        st->state = newval;
        ProcessMutexUnlock(st->mutex);
        return oldval;
    } else {
        ProcessMutexFixLockErr(st->mutex, err);
        return shmmbuf_state_set(st, newval);
    }

    SHMMBUF_VERIFY_STATE(oldval);
    return oldval;
}


NOWARNING_UNUSED(static)
size_t shmmbuf_state_comp_exch (shmmbuf_state_t *st, size_t comp, size_t exch)
{
    size_t oldval = SHMMBUF_INVALID_STATE;
    int err = ProcessMutexLock(st->mutex);

    if (! err) {
        oldval = st->state;
        if (st->state == comp) {
            st->state = exch;
        }
        ProcessMutexUnlock(st->mutex);
        return oldval;
    } else {
        ProcessMutexFixLockErr(st->mutex, err);
        return shmmbuf_state_comp_exch(st, comp, exch);
    }

    SHMMBUF_VERIFY_STATE(oldval);
    return oldval;
}


typedef struct _shmmbuf_semaphore_t
{
    HANDLE  nonzero;
    int maxcount;
} shmmbuf_semaphore_t, *shmmbuf_semaphore;


NOWARNING_UNUSED(static)
void shmmbuf_semaphore_init (shmmbuf_semaphore_t *semap, int maxcount, const char *name)
{
    DWORD  dwErr;
    HANDLE semaph;

    semaph = CreateSemaphoreA(NULL, 0, maxcount, name);
    if (! semaph) {
        exit(EXIT_FAILURE);
    }

    dwErr = GetLastError();
    if ( dwErr && dwErr != ERROR_ALREADY_EXISTS ) {
        CloseHandle(semaph);
        exit(EXIT_FAILURE);
    }

    /* success */
    semap->maxcount = maxcount;
    semap->nonzero = semaph;
}


NOWARNING_UNUSED(static)
int shmmbuf_semaphore_post (shmmbuf_semaphore_t * semap, size_t timeout_us)
{
    return ReleaseSemaphore(semap->nonzero, 1, 0)? 0 : (-1);
}


NOWARNING_UNUSED(static)
int shmmbuf_semaphore_wait (shmmbuf_semaphore_t * semap, size_t timeout_us)
{
    int ret = -1;

    if (timeout_us == SHMMBUF_TIMEOUT_INFINITE) {
        ret = WaitForSingleObject(semap->nonzero, INFINITE);
    } else {
        ret = WaitForSingleObject(semap->nonzero, (DWORD)(timeout_us/1000UL));
    }

    return ret;
}


/**
 * shared memory mmap file with a layout of ring buffer.
 * See also:
 *   https://github.com/pepstack/clogger/blob/master/src/ringbuf.h
 */
typedef struct _shmmap_buffer_t
{
    /**
     * total size in bytes for this shmmap file
     *   struct _shmmap_buffer_t *this;
     *   shmfilesize = sizeof(*this) + this->Length
     */
    size_t shmfilesize;

    /**
     * magic ^ token => cipher
     * cipher ^ token == magic
     */
    ub8token_t magic;
    ub8token_t cipher;

    shmmbuf_semaphore_t  semaphore;

    /* Read Lock */
    HANDLE RLock;

    /* Write Lock */
    HANDLE WLock;

    /* Write Offset to the Buffer start */
    shmmbuf_state_t WOffset;

    /* Read Offset to the Buffer start */
    shmmbuf_state_t ROffset;

    /* Length of ring Buffer: total size in bytes */
    size_t Length;

    /* ring buffer in shared memory with Length */
    char Buffer[0];
} shmmap_buffer_t;


#define SHMRINGBUF_RESTORE_WRAP(Ro, Wo, L)  \
    ((ssize_t)((Ro)/(L) == (Wo)/(L) ? 0 : 1))


#define SHMRINGBUF_RESTORE_STATE(Ro, Wo, L)  \
    ssize_t wrap = SHMRINGBUF_RESTORE_WRAP(Ro, Wo, L); \
    ssize_t R = (ssize_t)((Ro) % (L)); \
    ssize_t W = (ssize_t)((Wo) % (L))


#define SHMRINGBUF_NORMALIZE_OFFSET(Ao, L)  \
    ((((Ao)/(L))%2)*L + (Ao)%(L))


NOWARNING_UNUSED(static)
int __shmmap_buffer_read_internal (shmmap_buffer_t *shmbuf, ssize_t wrap, ssize_t R, ssize_t W, ssize_t L, int (*nextentry_cb)(const shmmbuf_entry_t *, void *), void *arg);


NOWARNING_UNUSED(static)
void shmmap_buffer_close (shmmap_buffer_t *shmbuf)
{
    CloseHandle(shmbuf->RLock);
    CloseHandle(shmbuf->WLock);

    CloseHandle(shmbuf->WOffset.mutex);
    CloseHandle(shmbuf->ROffset.mutex);

    CloseHandle(shmbuf->semaphore.nonzero);

    UnmapViewOfFile(shmbuf);
}


NOWARNING_UNUSED(static)
int shmmap_buffer_delete (const char *shmfilename)
{
    return DeleteFileA(shmfilename)? 0 : (-1);
}


NOWARNING_UNUSED(static)
int shmmap_verify_token (shmmap_buffer_t *shmbuf, ub8token_t *token, ub8token_t (decipher_cb)(const ub8token_t, ub8token_t *))
{
    ub8token_t magic;

    if (! shmbuf->cipher) {
        /* true: cipher disabled */
        return 1;
    }

    if (! token) {
        /* true: token not given */
        return 0;
    }

    /* cipher and token are all given */
    if (decipher_cb) {
        magic = decipher_cb(shmbuf->cipher, token);
    } else {
        magic = shmmbuf_decipher_token(shmbuf->cipher, token);
    }

    if (! memcmp(&magic, &shmbuf->magic, sizeof(shmbuf->magic))) {
        /* validate success when: magic == shmbuf->magic */
        return 1;
    }

    /* validate failed */
    return 0;
}


/**
 * shmmap_buffer_create()
 *   Create if not exists or open an existing shmmap file.
 *
 * Params:
 *   token - Point to a buffer with 8 bytes-length taken a plain token text.
 *           Whether the content in token changes or not will depend on the
 *            implementation of cipher_cb.
 *
 *   cipher_cb - A cryptography callback provided by caller to generate an
 *                encrypted cipher with 8 bytes-length from given token.
 *
 *    Below sample creates a 4 MB encrypted shmmap buffer using token number:
 *
 *       const char  shmname[] = "shmmap-sample";
 *       char        msgbuf[4096];
 *       ub8token_t  token = 12345678;
 *       size_t      bsize = 4*1024*1024;
 *
 *       // producer application
 *       void produce (void) {
 *           int wok;
 *           shmmap_buffer_t *shmb;
 *
 *           if (shmmap_buffer_create(&shmb, shmname, 0666, bsize, &token, 0, 0) == SHMMBUF_CREATE_SUCCESS) {
 *               wok = shmmap_buffer_write(shmb, "Hello Shmmap Sample", strlen("Hello Shmmap Sample"));
 *               // ...
 *               shmmap_buffer_close(shmb);
 *           }
 *       }
 *
 *       // consumer application
 *       void consume (void) {
 *           size_t rdlen;
 *           shmmap_buffer_t *shmb;
 *
 *           if (shmmap_buffer_create(&shmb, shmname, 0666, 0, &token, 0, 0) == SHMMBUF_CREATE_SUCCESS) {
 *               rdlen = shmmap_buffer_read_copy(shmb, msgbuf, sizeof(msgbuf));
 *               // ...
 *               shmmap_buffer_close(shmb);
 *           }
 *       }
 *
 * Returns:
 *   see Returns of shmmap_buffer_create() in above
 */
NOWARNING_UNUSED(static)
int shmmap_buffer_create (shmmap_buffer_t **outshmbuf, const char *shmfilename, int filemode, size_t filesize,
    ub8token_t *token,
    ub8token_t (*encipher_cb)(const ub8token_t magic, ub8token_t *token),
    ub8token_t (*decipher_cb)(const ub8token_t magic, ub8token_t *token))
{
    shmmap_buffer_t *shmbuf;

    char *nameprefix;
    char namebuf[128] = {0};

    /* total size in bytes of shmmap file */
    size_t mapfilesize = 0;

    /* aligned shared memory size as Length of ring buffer */
    size_t bufferLength = 0;

    int exist = 0;

    if (filesize == SHMMBUF_INVALID_STATE) {
        printf("(shmmbuf-win.h:%d) invalid filesize.\n", __LINE__);
        return SHMMBUF_CREATE_ERRSIZE;
    }

    /* create or open mapping file */
    HANDLE hshmfile;
    HANDLE hfilemapping;

    /* Default permission required to read and write to the shared memory. */
	DWORD protection = PAGE_READWRITE;

    /* Comment out this line to enable page caching, which delivers a
	    * slightly better performance, but which prevents the logger from
	    * immediately seeing changes (meaning that the last range of data
	    * written to shared memory before a crash may end up being lost --
	    * exercise caution when commenting out these flags).
	    */
	protection |= SEC_COMMIT | SEC_NOCACHE;

    if (filesize) {
        bufferLength = SHMMBUF_ALIGN_PAGESIZE(filesize);
        mapfilesize = SHMMBUF_BUFFER_HDRSIZE + bufferLength;
    }

    hshmfile = CreateFileA(shmfilename, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                    CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

    if (hshmfile == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_EXISTS) {
        hshmfile = CreateFileA(shmfilename, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
        exist = 1;
    }

    if (hshmfile == INVALID_HANDLE_VALUE) {
        printf("(shmmbuf-win.h:%d) CreateFile failed: %s\n", __LINE__, shmfilename);
        return SHMMBUF_CREATE_ERROPEN;
    }

    /* Creates or opens a named or unnamed file mapping object for a specified file.
        * If the object exists before the function call, the function returns a handle
        * to the existing object (with its current size, not the specified size)
        */
    if (exist) {
        hfilemapping = CreateFileMappingA(hshmfile, NULL, protection,
                            (DWORD) 0,
                            (DWORD) SHMMBUF_BUFFER_HDRSIZE,
                            NULL);
    } else {
        hfilemapping = CreateFileMappingA(hshmfile, NULL, protection,
                            (DWORD)((ub8)mapfilesize >> 32),
                            (DWORD)((ub8)mapfilesize & 0xffffffff),
                            NULL);
    }

    if (! hfilemapping) {
        printf("(shmmbuf-win.h:%d) CreateFileMapping failed: %s\n", __LINE__, shmfilename);
        CloseHandle(hshmfile);
        return SHMMBUF_CREATE_ERROPEN;
    }

    if (exist) {
        /* Check whether the buffer already exists.  If so, another app
	     * has already created the buffer (possibly due to two instances
	     * of the logging utility), so we should not clear out the header
         * at the start of shared memory.
         */
        shmbuf = (shmmap_buffer_t *) MapViewOfFile(hfilemapping, FILE_MAP_ALL_ACCESS, 0, 0, SHMMBUF_BUFFER_HDRSIZE);
        if (! shmbuf) {
            CloseHandle(hfilemapping);
            CloseHandle(hshmfile);
            return SHMMBUF_CREATE_ERRMMAP;
        }

        if (! filesize) {
            bufferLength = shmbuf->Length;
            mapfilesize = shmbuf->shmfilesize;
        }

        if (mapfilesize != shmbuf->shmfilesize || bufferLength != shmbuf->Length) {
            UnmapViewOfFile(shmbuf);
            CloseHandle(hfilemapping);
            CloseHandle(hshmfile);
            return SHMMBUF_CREATE_ERRSIZE;
        }

        if (! shmmap_verify_token(shmbuf, token, decipher_cb)) {
            UnmapViewOfFile(shmbuf);
            CloseHandle(hfilemapping);
            CloseHandle(hshmfile);
            return SHMMBUF_CREATE_ERRTOKEN;
        }

        UnmapViewOfFile(shmbuf);
        CloseHandle(hfilemapping);

        /* reopen again */
        hfilemapping = CreateFileMappingA(hshmfile, NULL, protection,
                (DWORD)((ub8)mapfilesize >> 32),
                (DWORD)((ub8)mapfilesize & 0xffffffff),
                NULL);
        if (! hfilemapping) {
            CloseHandle(hshmfile);
            return SHMMBUF_CREATE_ERROPEN;
        }
    }

    CloseHandle(hshmfile);

    if (mapfilesize < SHMMBUF_BUFFER_HDRSIZE + SHMMBUF_ALIGN_PAGESIZE(0)) {
        CloseHandle(hfilemapping);
        return SHMMBUF_CREATE_ERRSIZE;
    }

    shmbuf = (shmmap_buffer_t *) MapViewOfFile(hfilemapping, FILE_MAP_ALL_ACCESS, (DWORD)0, (DWORD)0, (SIZE_T)mapfilesize);
    if (! shmbuf) {
        CloseHandle(hfilemapping);
        return SHMMBUF_CREATE_ERRMMAP;
    }

    CloseHandle(hfilemapping);

    nameprefix = strrchr(shmfilename, '/');
    if (!nameprefix) {
        nameprefix = strrchr(shmfilename, '\\');
    }
    if (!nameprefix) {
        nameprefix = (char *) shmfilename;
    } else {
        nameprefix++;
    }

    if (! exist) {
        randctx64       rctx;
        struct timespec now;
        __ub8_t         seedus;

        bzero(shmbuf, mapfilesize);

        shmmap_gettimeofday(&now);

        seedus = now.tv_sec;
        seedus *= 1000000UL;
        seedus += (now.tv_nsec / 1000UL);

        randctx64_init(&rctx, seedus);

        shmbuf->magic = rand64_gen_int(&rctx, 0x0111111111111111, 0x1fffffffffffffff);

        if (token) {
            if (encipher_cb) {
                shmbuf->cipher = encipher_cb(shmbuf->magic, token);
            } else {
                shmbuf->cipher = shmmbuf_encipher_token(shmbuf->magic, token);
            }
        }

        shmbuf->Length = bufferLength;
        shmbuf->shmfilesize = mapfilesize;
    }

    snprintf_chkd_V1(namebuf, 127, "SHMMAP.%s.semaphore", nameprefix);
    shmmbuf_semaphore_init(&shmbuf->semaphore, SHMMBUF_SEMAPHORE_MAXVAL, namebuf);

    snprintf_chkd_V1(namebuf, 127, "SHMMAP.%s.readlock", nameprefix);
    ProcessMutexInit(&shmbuf->RLock, namebuf);

    snprintf_chkd_V1(namebuf, 127, "SHMMAP.%s.writelock", nameprefix);
    ProcessMutexInit(&shmbuf->WLock, namebuf);

    snprintf_chkd_V1(namebuf, 127, "SHMMAP.%s.writeofflock", nameprefix);
    ProcessMutexInit(&shmbuf->WOffset.mutex, namebuf);

    snprintf_chkd_V1(namebuf, 127, "SHMMAP.%s.readofflock", nameprefix);
    ProcessMutexInit(&shmbuf->ROffset.mutex, namebuf);

    if (! shmmap_verify_token(shmbuf, token, decipher_cb)) {
        shmmap_buffer_close(shmbuf);
        return SHMMBUF_CREATE_ERRTOKEN;
    }

    /* create or open success */
    *outshmbuf = shmbuf;
    return SHMMBUF_CREATE_SUCCESS;
}


/**
 * shmmap_buffer_write()
 *   Write chunk data of entry into shmmap ring buffer.
 *
 * Returns:
 *   SHMMBUF_WRITE_SUCCESS(1) - write success
 *   SHMMBUF_WRITE_AGAIN(0)   - write again
 *   SHMMBUF_WRITE_FATAL(-1)  - fatal write error
 */
NOWARNING_UNUSED(static)
int shmmap_buffer_write (shmmap_buffer_t *shmbuf, const void *chunk, size_t chunksz)
{
    shmmbuf_entry_t *entry;
    ssize_t Ro, Wo,
        L = (ssize_t)shmbuf->Length,
        AENTSZ = (ssize_t)SHMMBUF_ALIGN_ENTRYSIZE(chunksz);

    if (! AENTSZ || AENTSZ == (ssize_t) SHMMBUF_INVALID_STATE || AENTSZ > (ssize_t) (L / SHMMBUF_ENTRY_HDRSIZE)) {
        /* fatal error should not occurred! */
    # ifdef SHMMBUF_TRACE_PRINT_ON
            printf("(shmmbuf.h:%d) fatal error: invalid chunksz(=%" PRIu64").\n", __LINE__, chunksz);
    # endif
        return SHMMBUF_WRITE_FATAL;
    }

    if (ProcessMutexTryLock(shmbuf->WLock) == 0) {
        /* Get original ROffset */
        Ro = shmmbuf_state_get(&shmbuf->ROffset);
        Wo = shmbuf->WOffset.state;

        SHMRINGBUF_RESTORE_STATE(Ro, Wo, L);

    # ifdef SHMMBUF_TRACE_PRINT_ON
        if (wrap) {
            printf("(shmmbuf.h:%d) shmmap_buffer_write(%" PRIu64":%" PRIu64"): W=%" PRId64" R=%" PRId64" L=%" PRId64"\n",
                __LINE__, chunksz, AENTSZ, W, R, L);
        } else {
            printf("(shmmbuf.h:%d) shmmap_buffer_write(%" PRIu64":%" PRIu64"): R=%" PRId64" W=%" PRId64" L=%" PRId64"\n",
                __LINE__, chunksz, AENTSZ, R, W, L);
        }
    # endif

        /* Sw = L - (wrap*L + W - R) */
        if (L - (wrap*L + W - R) >= AENTSZ) {
            if (wrap) { /* wrap(1): 0 .. W < R < L */
                entry = SHMMBUF_ENTRY_CAST(&shmbuf->Buffer[W]);
                entry->size = chunksz;
                memcpy(entry->chunk, chunk, chunksz);

                /* WOffset = Wo + AENTSZ */
                W = SHMRINGBUF_NORMALIZE_OFFSET(Wo + AENTSZ, L);
                shmmbuf_state_set(&shmbuf->WOffset, W);
            } else {   /* wrap(0): 0 .. R < W < L */
                if (L - W >= AENTSZ) {
                    entry = SHMMBUF_ENTRY_CAST(&shmbuf->Buffer[W]);
                    entry->size = chunksz;
                    memcpy(entry->chunk, chunk, chunksz);

                    /* WOffset = Wo + AENTSZ */
                    W = SHMRINGBUF_NORMALIZE_OFFSET(Wo + AENTSZ, L);
                    shmmbuf_state_set(&shmbuf->WOffset, W);
                } else if (R - 0 >= AENTSZ) {
                    /* clear W slot before wrap W */
                    bzero(&shmbuf->Buffer[W], L - W);

                    /* wrap W to 0 */
                    entry = SHMMBUF_ENTRY_CAST(&shmbuf->Buffer[0]);
                    entry->size = chunksz;
                    memcpy(entry->chunk, chunk, chunksz);

                    /* WOffset = AENTSZ, wrap = 1 */
                    W = AENTSZ + (1 - (int)(Ro/L))*L;

                    shmmbuf_state_set(&shmbuf->WOffset, W);
                } else {
                    /* no space left to write, expect to call again */
                    ProcessMutexUnlock(shmbuf->WLock);
                    return SHMMBUF_WRITE_AGAIN;
                }
            }

            ProcessMutexUnlock(shmbuf->WLock);
            return SHMMBUF_WRITE_SUCCESS;
        }

        /* no space left to write */
        ProcessMutexUnlock(shmbuf->WLock);
        return SHMMBUF_WRITE_AGAIN;
    }

    /* lock fail to write, expect to call again */
    return SHMMBUF_WRITE_AGAIN;
}


/**
 * shmmap_buffer_read_copy()
 *   Copy entry from shmmap ringbuffer into rdbuf.
 *
 * returns:
 *   SHMMBUF_READ_AGAIN(0)       - read again
 *   SHMMBUF_READ_FATAL(-1)      - fatal read error
 *
 *   ret > 0 and ret <= rdbufsz - read success
 *   ret > rdbufsz              - no read for insufficient buffer
 */
NOWARNING_UNUSED(static)
size_t shmmap_buffer_read_copy (shmmap_buffer_t *shmbuf, char *rdbuf, size_t rdbufsz)
{
    shmmbuf_entry_t *entry;

    ssize_t Ro, Wo, entsize, AENTSZ,
            L = (ssize_t)shmbuf->Length,
            HENTSZ = (ssize_t)SHMMBUF_ALIGN_ENTRYSIZE(0);

    if (ProcessMutexTryLock(shmbuf->RLock) == 0) {
        Wo = shmmbuf_state_get(&shmbuf->WOffset);
        Ro = shmbuf->ROffset.state;

        SHMRINGBUF_RESTORE_STATE(Ro, Wo, L);

    # ifdef SHMMBUF_TRACE_PRINT_ON
        if (wrap) {
            printf("(shmmbuf.h:%d) shmmap_buffer_read_copy(): W=%" PRId64" R=%" PRId64" L=%" PRId64"\n", __LINE__, W, R, L);
        } else {
            printf("(shmmbuf.h:%d) shmmap_buffer_read_copy(): R=%" PRId64" W=%" PRId64" L=%" PRId64"\n", __LINE__, R, W, L);
        }
    # endif

        /* Sr = f*L + W - R */
        if (wrap*L + W - R > HENTSZ) {
            if (wrap) {  /* wrap(1): 0 .. W < R < L */
                if (L - R > HENTSZ) {
                    entry = SHMMBUF_ENTRY_CAST(&shmbuf->Buffer[R]);
                    if (entry->size) {
                        AENTSZ = SHMMBUF_ALIGN_ENTRYSIZE(entry->size);

                        if (L - R >= AENTSZ) {
                            entsize = entry->size;
                            if (entsize > (ssize_t) rdbufsz) {
                                /* read buf is insufficient for entry.
                                 * read nothing if returned entsize > rdbufsz */
                                ProcessMutexUnlock(shmbuf->RLock);
                                return (size_t)entsize;
                            }

                            /* read entry chunk into rdbuf ok */
                            memcpy(rdbuf, entry->chunk, entsize);

                            R = SHMRINGBUF_NORMALIZE_OFFSET(Ro + AENTSZ, L);
                            shmmbuf_state_set(&shmbuf->ROffset, R);

                            /* read success if returned entsize <= rdbufsz */
                            ProcessMutexUnlock(shmbuf->RLock);
                            return (size_t)entsize;
                        }

                        printf("(shmmbuf.h:%d) SHOULD NEVER RUN TO THIS! fatal bug.\n", __LINE__);
                        ProcessMutexUnlock(shmbuf->RLock);
                        return SHMMBUF_READ_FATAL;
                    } else {
                        /* reset ROffset to 0 (set wrap = 0) */
                        shmmbuf_state_set(&shmbuf->ROffset, (Wo/L) * L);

                        /* expect to read again */
                        ProcessMutexUnlock(shmbuf->RLock);
                        return shmmap_buffer_read_copy(shmbuf, rdbuf, rdbufsz);
                    }
                } else if (W - 0 > HENTSZ) {
                    /* reset ROffset to 0 */
                    entry = SHMMBUF_ENTRY_CAST(&shmbuf->Buffer[0]);
                    if (entry->size) {
                        AENTSZ = SHMMBUF_ALIGN_ENTRYSIZE(entry->size);

                        if (W - 0 >= AENTSZ) {
                            entsize = entry->size;
                            if (entsize > (ssize_t) rdbufsz) {
                                /* read buf is insufficient for entry.
                                 * read nothing if returned entsize > rdbufsz */
                                ProcessMutexUnlock(shmbuf->RLock);
                                return (size_t)entsize;
                            }

                            /* read entry chunk into rdbuf ok */
                            memcpy(rdbuf, entry->chunk, entsize);

                            /* ROffset = AENTSZ, wrap = 0 */
                            shmmbuf_state_set(&shmbuf->ROffset, AENTSZ + (Wo/L)*L);

                            /* read success if returned entsize <= rdbufsz */
                            ProcessMutexUnlock(shmbuf->RLock);
                            return (size_t)entsize;
                        }
                    }

                    printf("(shmmbuf.h:%d) SHOULD NEVER RUN TO THIS! fatal bug.\n", __LINE__);
                    ProcessMutexUnlock(shmbuf->RLock);
                    return SHMMBUF_READ_FATAL;
                }
            } else {  /* wrap(0): 0 .. R < W < L */
                entry = SHMMBUF_ENTRY_CAST(&shmbuf->Buffer[R]);
                if (entry->size) {
                    AENTSZ = SHMMBUF_ALIGN_ENTRYSIZE(entry->size);

                    if (W - R >= AENTSZ) {
                        entsize = entry->size;
                        if (entsize > (ssize_t) rdbufsz) {
                            /* read buf is insufficient for entry.
                             * read nothing if returned entsize > rdbufsz
                             */
                            ProcessMutexUnlock(shmbuf->RLock);
                            return (size_t)entsize;
                        }

                        /* read entry chunk into rdbuf ok */
                        memcpy(rdbuf, entry->chunk, entsize);

                        R = SHMRINGBUF_NORMALIZE_OFFSET(Ro + AENTSZ, L);
                        shmmbuf_state_set(&shmbuf->ROffset, R);

                        /* read success if returned entsize <= rdbufsz */
                        ProcessMutexUnlock(shmbuf->RLock);
                        return (size_t)entsize;
                    }
                }

                printf("(shmmbuf.h:%d) SHOULD NEVER RUN TO THIS! fatal bug.\n", __LINE__);
                ProcessMutexUnlock(shmbuf->RLock);
                return SHMMBUF_READ_FATAL;
            }
        }

        /* no entry to read, retry again */
        ProcessMutexUnlock(shmbuf->RLock);
        return SHMMBUF_READ_AGAIN;
    }

    /* read locked fail, retry again */
    return SHMMBUF_READ_AGAIN;
}


/**
 * shmmap_buffer_read_next()
 *   Read next one entry from shmmap ring buffer into callback (no copy data).
 *
 * params:
 *   nextentry_cb() - Callback implemented by caller should ONLY
 *                     return SHMMBUF_READ_NEXT(1) or SHMMBUF_READ_AGAIN(0)
 *                    DO NOT change and members of entry in nextentry_cb().
 * returns:
 *   SHMMBUF_READ_NEXT(1)    - read for next one
 *   SHMMBUF_READ_AGAIN(0)   - read current one again
 *   SHMMBUF_READ_FATAL(-1)  - fatal read error
 */
NOWARNING_UNUSED(static)
int shmmap_buffer_read_next (shmmap_buffer_t *shmbuf, int (*nextentry_cb)(const shmmbuf_entry_t *entry, void *arg), void *arg)
{
    int ret = SHMMBUF_READ_AGAIN;

    if (ProcessMutexTryLock(shmbuf->RLock) == 0) {
        ssize_t Wo = shmmbuf_state_get(&shmbuf->WOffset);
        ssize_t Ro = shmbuf->ROffset.state;
        ssize_t wrap = SHMRINGBUF_RESTORE_WRAP(Ro, Wo, shmbuf->Length);

        ret = __shmmap_buffer_read_internal(shmbuf, wrap, Ro, Wo, (ssize_t)shmbuf->Length, nextentry_cb, arg);

        ProcessMutexUnlock(shmbuf->RLock);
    }

    return ret;
}



/**
 * shmmap_buffer_read_next_batch()
 *   Read next batch entries from shmmap ring buffer into callback (no copy data).
 */
NOWARNING_UNUSED(static)
int shmmap_buffer_read_next_batch (shmmap_buffer_t *shmbuf, int (*nextentry_cb)(const shmmbuf_entry_t *entry, void *arg), void *arg, int batch)
{
    int num = 0, ret = SHMMBUF_READ_AGAIN;

    if (ProcessMutexTryLock(shmbuf->RLock) == 0) {
        ssize_t wrap, Wo, Ro;

        while (batch-- > 0) {
            Wo = shmmbuf_state_get(&shmbuf->WOffset);
            Ro = shmbuf->ROffset.state;
            wrap = SHMRINGBUF_RESTORE_WRAP(Ro, Wo, shmbuf->Length);

            ret = __shmmap_buffer_read_internal(shmbuf, wrap, Ro, Wo, (ssize_t)shmbuf->Length, nextentry_cb, arg);
            if (ret == SHMMBUF_READ_NEXT) {
                num++;
                continue;
            }

            if (ret == SHMMBUF_READ_AGAIN) {
                /* stop current batch */
                break;
            }

            if (ret == SHMMBUF_READ_FATAL) {
                num = (int) SHMMBUF_READ_FATAL;
                break;
            }

            /* SHOULD NEVER RUN TO THIS ! */
        }

        ProcessMutexUnlock(shmbuf->RLock);
    }

    return num;
}


NOWARNING_UNUSED(static)
int shmmap_buffer_post (shmmap_buffer_t *shmbuf, size_t timeout_us)
{
    return shmmbuf_semaphore_post(&shmbuf->semaphore, timeout_us);
}


NOWARNING_UNUSED(static)
int shmmap_buffer_wait (shmmap_buffer_t *shmbuf, size_t timeout_us)
{
    return shmmbuf_semaphore_wait(&shmbuf->semaphore, timeout_us);
}


/**
 * shmmap_buffer_force_unlock()
 *   Force unlock state lock. statelock can be one or combination of below:
 *
 *     SHMMBUF_READSTATE_LOCK
 *     SHMMBUF_WRITESTATE_LOCK
 * NOTES:
 *   Make sure prior to force unlocking statelock, all the applications using
 *    the same shmmap file should be paused!
 */
#define SHMMBUF_READSTATE_LOCK    1
#define SHMMBUF_WRITESTATE_LOCK   2

NOWARNING_UNUSED(static)
void shmmap_buffer_force_unlock (shmmap_buffer_t *shmbuf, int statelock)
{
    if (SHMMBUF_READSTATE_LOCK & statelock) {
        ProcessMutexUnlock(shmbuf->RLock);
    }

    if (SHMMBUF_WRITESTATE_LOCK & statelock) {
        ProcessMutexUnlock(shmbuf->WLock);
    }
}


/**
 * __shmmap_buffer_read_internal()
 *   Private function. do not call it in your code !!
 */
int __shmmap_buffer_read_internal (shmmap_buffer_t *shmbuf, ssize_t wrap, ssize_t Ro, ssize_t Wo, ssize_t L, int (*nextentry_cb)(const shmmbuf_entry_t *, void *), void *arg)
{
    shmmbuf_entry_t *entry;

    ssize_t AENTSZ,
            HENTSZ = (ssize_t)SHMMBUF_ALIGN_ENTRYSIZE(0);

    ssize_t R = Ro % L;
    ssize_t W = Wo % L;

# ifdef SHMMBUF_TRACE_PRINT_ON
    if (wrap) {
        printf("(shmmbuf.h:%d) shmmap_buffer_read_next(): W=%" PRId64" R=%" PRId64" L=%" PRId64"\n", __LINE__, W, R, L);
    } else {
        printf("(shmmbuf.h:%d) shmmap_buffer_read_next(): R=%" PRId64" W=%" PRId64" L=%" PRId64"\n", __LINE__, R, W, L);
    }
# endif

    /* Sr = f*L + W - R */
    if (wrap*L + W - R > HENTSZ) {
        if (wrap) {  /* wrap(1): 0 .. W < R < L */
            if (L - R > HENTSZ) {
                entry = SHMMBUF_ENTRY_CAST(&shmbuf->Buffer[R]);
                if (entry->size) {
                    AENTSZ = SHMMBUF_ALIGN_ENTRYSIZE(entry->size);

                    if (L - R >= AENTSZ) {
                        if (nextentry_cb(entry, arg)) {
                            /* read success and set ROffset to next entry */
                            R = SHMRINGBUF_NORMALIZE_OFFSET(Ro + AENTSZ, L);
                            shmmbuf_state_set(&shmbuf->ROffset, R);

                            return SHMMBUF_READ_NEXT;
                        } else {
                            /* read paused by caller */
                            return SHMMBUF_READ_AGAIN;
                        }
                    }

                    printf("(shmmbuf.h:%d) SHOULD NEVER RUN TO THIS! fatal bug.\n", __LINE__);
                    return SHMMBUF_READ_FATAL;
                } else {
                    /* reset ROffset to 0 (set wrap = 0) */
                    shmmbuf_state_set(&shmbuf->ROffset, (Wo/L) * L);

                    return shmmap_buffer_read_next(shmbuf, nextentry_cb, arg);
                }
            } else if (W - 0 > HENTSZ) {
                /* reset ROffset to 0 */
                entry = SHMMBUF_ENTRY_CAST(&shmbuf->Buffer[0]);
                if (entry->size) {
                    AENTSZ = SHMMBUF_ALIGN_ENTRYSIZE(entry->size);

                    if (W - 0 >= AENTSZ) {
                        if (nextentry_cb(entry, arg)) {
                            /* read success and set ROffset to next entry */
                            shmmbuf_state_set(&shmbuf->ROffset, AENTSZ + (Wo/L)*L);

                            return SHMMBUF_READ_NEXT;
                        } else {
                            /* read paused by caller */
                            return SHMMBUF_READ_AGAIN;
                        }
                    }
                }

                printf("(shmmbuf.h:%d) SHOULD NEVER RUN TO THIS! fatal bug.\n", __LINE__);
                return SHMMBUF_READ_FATAL;
            }
        } else {  /* wrap(0): 0 .. R < W < L */
            entry = SHMMBUF_ENTRY_CAST(&shmbuf->Buffer[R]);
            if (entry->size) {
                AENTSZ = SHMMBUF_ALIGN_ENTRYSIZE(entry->size);

                if (W - R >= AENTSZ) {
                    if (nextentry_cb(entry, arg)) {
                        /* read success and set ROffset to next entry */
                        R = SHMRINGBUF_NORMALIZE_OFFSET(Ro + AENTSZ, L);
                        shmmbuf_state_set(&shmbuf->ROffset, R);

                        return SHMMBUF_READ_NEXT;
                    } else {
                        /* read paused by caller */
                        return SHMMBUF_READ_AGAIN;
                    }
                }
            }

            printf("(shmmbuf.h:%d) SHOULD NEVER RUN TO THIS! fatal bug.\n", __LINE__);
            return SHMMBUF_READ_FATAL;
        }
    }

    /* no entry to read, retry again */
    return SHMMBUF_READ_AGAIN;
}

#ifdef __cplusplus
}
#endif

#endif /* SHMMBUF_WIN_H__ */