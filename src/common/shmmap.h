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
 * @filename   shmmap.h
 *   MP and MT-safe shared memory IO for Linux (POSIX.1-2008, glibc 2.12+).
 *
 * @author     Liang Zhang <350137278@qq.com>
 * @version    1.0.0
 * @create     2020-05-01 12:46:50
 * @update     2020-05-19 18:45:08
 *
 * NOTES:
 *  Prior to include this file, define as following to enable
 *   trace print in spite of speed penalty.
 *
 * #define SHMMAP_TRACE_PRINT_ON
 * #include "shmmap.h"
 */
#ifndef SHMMAP_H__
#define SHMMAP_H__

#if defined(__cplusplus)
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef __STDC_FORMAT_MACROS
# define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include <fcntl.h>
#include <pthread.h>
#include <semaphore.h>


/**
 * random generator for cryptography
 */
#include "randctx.h"
#include "md5sum.h"
#include "unitypes.h"


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
 * Shared memory file lies in: "/dev/shm/shmmap-buffer-default"
 */
#define SHMMAP_FILENAME_DEFAULT        "shmmap-buffer-default"

#define SHMMAP_FILEMODE_DEFAULT        0666


/**
 * Below definitions SHOULD NOT be changed!
 */
#define SHMMAP_PAGE_SIZE        ((size_t)4096)

#define SHMMAP_INVALID_STATE    ((size_t)(-1))

#define SHMMAP_TIMEOUT_INFINITE ((size_t)(-1))
#define SHMMAP_TIMEOUT_NOWAIT   (0)

#define SHMMAP_ENTRY_HDRSIZE    (sizeof(shmmap_entry_t))

#define SHMMAP_ALIGN_BSIZE(bsz, alignsize)  \
            ((size_t)((((size_t)(bsz)+(alignsize)-1)/(alignsize))*(alignsize)))

#define SHMMAP_ALIGN_PAGESIZE(fsz)    \
            SHMMAP_ALIGN_BSIZE((fsz), SHMMAP_PAGE_SIZE)

#define SHMMAP_ALIGN_ENTRYSIZE(chunksz)    \
            SHMMAP_ALIGN_BSIZE((chunksz + SHMMAP_ENTRY_HDRSIZE), SHMMAP_ENTRY_HDRSIZE)

#define SHMMAP_BUFFER_HDRSIZE   \
            SHMMAP_ALIGN_BSIZE(sizeof(shmmap_buffer_t), SHMMAP_PAGE_SIZE)

#define SHMMAP_ENTRY_CAST(p)           ((shmmap_entry_t *)(p))

#define SHMMAP_VERIFY_STATE(val)    do { \
        if ((val) == SHMMAP_INVALID_STATE) { \
            exit(EXIT_FAILURE); \
        } \
    } while(0)


/**
 * Returns of shmmap_buffer_create()
 */
#define SHMMAP_CREATE_SUCCESS      0

#define SHMMAP_CREATE_ERROPEN     (1)
#define SHMMAP_CREATE_ERRMMAP     (2)
#define SHMMAP_CREATE_ERRSIZE     (3)
#define SHMMAP_CREATE_ERRTRUNC    (4)
#define SHMMAP_CREATE_ERRTOKEN    (5)


/**
 * Returns of shmmap_buffer_write()
 */
#define SHMMAP_WRITE_SUCCESS     ((int)(1))
#define SHMMAP_WRITE_AGAIN       ((int)(0))
#define SHMMAP_WRITE_FATAL       ((int)(-1))


/**
 * Returns of shmmap_buffer_read_?()
 */
#define SHMMAP_READ_NEXT         ((int)(1))
#define SHMMAP_READ_AGAIN        ((int)(0))
#define SHMMAP_READ_FATAL        ((int)(-1))


/**
 * default token cryptographic algorithm
 */
typedef __ub8_t ub8token_t;


NOWARNING_UNUSED(static)
ub8token_t shmmap_encipher_token (const ub8token_t magic, ub8token_t *token)
{
    ub8token_t cipher = magic ^ (*token);
    return cipher;
}


NOWARNING_UNUSED(static)
ub8token_t shmmap_decipher_token (const ub8token_t cipher, ub8token_t *token)
{
    ub8token_t magic = cipher ^ (*token);
    return magic;
}


/**
 * The layout of memory for any one entry in shmmap.
 */
typedef struct _shmmap_entry_t
{
    size_t size;
    char chunk[0];
} shmmap_entry_t;


/**
 * The atomic struct for state of shmmap.
 */
typedef struct _shmmap_state_t
{
    /* atomic state value */
    size_t state;

    /* process-wide state lock */
    pthread_mutex_t mutex;
} shmmap_state_t;


NOWARNING_UNUSED(static)
void shmmap_gettimeofday (struct timespec *now)
{
#if defined(__WINDOWS__)
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
#else
    clock_gettime(CLOCK_REALTIME, now);
#endif
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


/**
 * http://www.linuxhowtos.org/manpages/3/pthread_mutexattr_setrobust.htm
 */
NOWARNING_UNUSED(static)
void shmmap_mutex_consistent (pthread_mutex_t *mutexp)
{
    if (pthread_mutex_consistent(mutexp)) {
        perror("pthread_mutex_consistent");
        exit(EXIT_FAILURE);
    }
    if (pthread_mutex_unlock(mutexp)) {
        perror("pthread_mutex_unlock");
        exit(EXIT_FAILURE);
    }
}


NOWARNING_UNUSED(static)
void process_shared_mutex_init (pthread_mutex_t *mutexp)
{
    pthread_mutexattr_t attr;
    int err = pthread_mutexattr_init(&attr);
    if (err) {
        printf("(shmmap.h:%d) pthread_mutexattr_init error(%d): %s.\n", __LINE__, err, strerror(err));
        exit(EXIT_FAILURE);
    }
    err = pthread_mutexattr_setpshared(&attr, PTHREAD_PROCESS_SHARED);
    if (err) {
        printf("(shmmap.h:%d) pthread_mutexattr_setpshared error(%d): %s.\n", __LINE__, err, strerror(err));
        pthread_mutexattr_destroy(&attr);
        exit(EXIT_FAILURE);
    }
    err = pthread_mutexattr_setrobust(&attr, PTHREAD_MUTEX_ROBUST);
    if (err) {
        printf("(shmmap.h:%d) pthread_mutexattr_setrobust error(%d): %s.\n", __LINE__, err, strerror(err));
        pthread_mutexattr_destroy(&attr);
        exit(EXIT_FAILURE);
    }
    err = pthread_mutex_init(mutexp, &attr);
    if (err) {
        printf("(shmmap.h:%d) pthread_mutex_init error(%d): %s.\n", __LINE__, err, strerror(err));
        pthread_mutexattr_destroy(&attr);
        exit(EXIT_FAILURE);
    }
    pthread_mutexattr_destroy(&attr);
}


NOWARNING_UNUSED(static)
int process_shared_mutex_lock (pthread_mutex_t *mutexp, int istry)
{
    int err;

    if (istry) {
        err = pthread_mutex_trylock(mutexp);
    } else {
        err = pthread_mutex_lock(mutexp);
    }

    if (err == EOWNERDEAD) {
        shmmap_mutex_consistent(mutexp);
        err = process_shared_mutex_lock(mutexp, istry);
    }

    return err;
}


#define process_shared_mutex_unlock(mutexp)  \
    pthread_mutex_unlock(mutexp)


NOWARNING_UNUSED(static)
void shmmap_state_init (shmmap_state_t *st, size_t state)
{
    process_shared_mutex_init(&st->mutex);
    st->state = state;
}


NOWARNING_UNUSED(static)
int shmmap_state_uninit (shmmap_state_t *st)
{
    int err = pthread_mutex_destroy(&st->mutex);
    return err;
}


NOWARNING_UNUSED(static)
size_t shmmap_state_get (shmmap_state_t *st)
{
    size_t val = SHMMAP_INVALID_STATE;
    int err = pthread_mutex_lock(&st->mutex);

    if (! err) {
        val = st->state;
        pthread_mutex_unlock(&st->mutex);
        return val;
    } else if (err == EOWNERDEAD) {
        shmmap_mutex_consistent(&st->mutex);
        return shmmap_state_get(st);
    }

    printf("pthread_mutex_lock fatal(%d): %s.\n", err, strerror(err));
    SHMMAP_VERIFY_STATE(val);
    return val;
}


NOWARNING_UNUSED(static)
size_t shmmap_state_set (shmmap_state_t *st, size_t newval)
{
    size_t oldval = SHMMAP_INVALID_STATE;
    int err = pthread_mutex_lock(&st->mutex);

    if (! err) {
        oldval = st->state;
        st->state = newval;
        pthread_mutex_unlock(&st->mutex);
        return oldval;
    } else if (err == EOWNERDEAD) {
        shmmap_mutex_consistent(&st->mutex);
        return shmmap_state_set(st, newval);
    }

    printf("pthread_mutex_lock fatal(%d): %s.\n", err, strerror(err));
    SHMMAP_VERIFY_STATE(oldval);
    return oldval;
}


NOWARNING_UNUSED(static)
size_t shmmap_state_comp_exch (shmmap_state_t *st, size_t comp, size_t exch)
{
    size_t oldval = SHMMAP_INVALID_STATE;
    int err = pthread_mutex_lock(&st->mutex);

    if (! err) {
        oldval = st->state;
        if (st->state == comp) {
            st->state = exch;
        }
        pthread_mutex_unlock(&st->mutex);
        return oldval;
    } else if (err == EOWNERDEAD) {
        shmmap_mutex_consistent(&st->mutex);
        return shmmap_state_comp_exch(st, comp, exch);
    }

    printf("pthread_mutex_lock fatal(%d): %s.\n", err, strerror(err));
    SHMMAP_VERIFY_STATE(oldval);
    return oldval;
}


typedef struct _shmmap_semaphore_t
{
    pthread_mutex_t lock;
    pthread_cond_t  nonzero;
    ssize_t         count;
} shmmap_semaphore_t, *shmmap_semaphore;


NOWARNING_UNUSED(static)
void shmmap_semaphore_init (shmmap_semaphore_t *semap)
{
    pthread_mutexattr_t mattr;
    pthread_condattr_t cattr;

    int err = pthread_mutexattr_init(&mattr);
    if (err) {
        printf("(shmmap.h:%d) pthread_mutexattr_init error(%d): %s.\n", __LINE__, err, strerror(err));
        exit(EXIT_FAILURE);
    }
    err = pthread_condattr_init(&cattr);
    if (err) {
        printf("(shmmap.h:%d) pthread_condattr_init error(%d): %s.\n", __LINE__, err, strerror(err));
        pthread_mutexattr_destroy(&mattr);
        exit(EXIT_FAILURE);
    }
    err = pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);
    if (err) {
        printf("(shmmap.h:%d) pthread_mutexattr_setpshared error(%d): %s.\n", __LINE__, err, strerror(err));
        pthread_mutexattr_destroy(&mattr);
        pthread_condattr_destroy(&cattr);
        exit(EXIT_FAILURE);
    }
    err = pthread_mutexattr_setrobust(&mattr, PTHREAD_MUTEX_ROBUST);
    if (err) {
        printf("(shmmap.h:%d) pthread_mutexattr_setrobust error(%d): %s.\n", __LINE__, err, strerror(err));
        pthread_mutexattr_destroy(&mattr);
        pthread_condattr_destroy(&cattr);
        exit(EXIT_FAILURE);
    }
    err = pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
    if (err) {
        printf("(shmmap.h:%d) pthread_condattr_setpshared error(%d): %s.\n", __LINE__, err, strerror(err));
        pthread_mutexattr_destroy(&mattr);
        pthread_condattr_destroy(&cattr);
        exit(EXIT_FAILURE);
    }
    err = pthread_mutex_init(&semap->lock, &mattr);
    if (err) {
        printf("(shmmap.h:%d) pthread_mutex_init error(%d): %s.\n", __LINE__, err, strerror(err));
        pthread_mutexattr_destroy(&mattr);
        pthread_condattr_destroy(&cattr);
        exit(EXIT_FAILURE);
    }
    err = pthread_cond_init(&semap->nonzero, &cattr);
    if (err) {
        printf("(shmmap.h:%d) pthread_cond_init error(%d): %s.\n", __LINE__, err, strerror(err));
        pthread_mutexattr_destroy(&mattr);
        pthread_condattr_destroy(&cattr);
        pthread_mutex_destroy(&semap->lock);
        exit(EXIT_FAILURE);
    }

    /* success */
    semap->count = 0;
}


NOWARNING_UNUSED(static)
int shmmap_semaphore_lock (shmmap_semaphore_t * semap, size_t timeout_us)
{
    if (timeout_us == SHMMAP_TIMEOUT_NOWAIT) {
        return pthread_mutex_trylock(&semap->lock);
    } else if (timeout_us == SHMMAP_TIMEOUT_INFINITE) {
        return pthread_mutex_lock(&semap->lock);
    } else {
        struct timespec now, abstime;
        shmmap_gettimeofday(&now);
        abstime.tv_sec = now.tv_sec + timeout_us / 1000000UL;
        abstime.tv_nsec = now.tv_nsec + (timeout_us % 1000000UL) * 1000UL;
        return pthread_mutex_timedlock(&semap->lock, &abstime);
    }
}


NOWARNING_UNUSED(static)
int shmmap_semaphore_post (shmmap_semaphore_t * semap, size_t timeout_us)
{
    int err = shmmap_semaphore_lock(semap, timeout_us);
    if (! err) {
        if (semap->count == 0) {
            pthread_cond_signal(&semap->nonzero);
        }
        semap->count++;

        return pthread_mutex_unlock(&semap->lock);
    } else if (err == EOWNERDEAD) {
        shmmap_mutex_consistent(&semap->lock);
        return shmmap_semaphore_post(semap, timeout_us);
    } else if (err == EEXIST) {
        /* DO NOTHING ! */
    } else if (err != EBUSY && err != ETIMEDOUT) {
        /* SHOULD NEVER RUN TO THIS! */
        perror("pthread_mutex_lock");
    }
    return err;
}


NOWARNING_UNUSED(static)
int shmmap_semaphore_wait (shmmap_semaphore_t * semap, size_t timeout_us)
{
    int err = shmmap_semaphore_lock(semap, timeout_us);
    if (! err) {
        while (semap->count == 0) {
            if (timeout_us == SHMMAP_TIMEOUT_INFINITE) {
                pthread_cond_wait(&semap->nonzero, &semap->lock);
                continue;
            } else {
                struct timespec now, abstime;
                shmmap_gettimeofday(&now);
                abstime.tv_sec = now.tv_sec + timeout_us / 1000000UL;
                abstime.tv_nsec = now.tv_nsec + (timeout_us % 1000000UL) * 1000UL;

                pthread_cond_timedwait(&semap->nonzero, &semap->lock, &abstime);
                break;
            }
        }

        if (semap->count > 0) {
            semap->count--;
        }
        return pthread_mutex_unlock(&semap->lock);
    } else if (err == EOWNERDEAD) {
        shmmap_mutex_consistent(&semap->lock);
        return shmmap_semaphore_wait(semap, timeout_us);
    }  else if (err == EEXIST) {
        /* DO NOTHING ! */
    } else if (err != EBUSY && err != ETIMEDOUT) {
        /* can go here */
        perror("pthread_mutex_lock");
    }
    return err;
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

    /**
     * https://linux.die.net/man/3/pthread_mutexattr_init
     */
    shmmap_semaphore_t  semaphore;

    /**
     * RingBuffer
     *
     * Length(L) = 10, Read(R), Write(W), wrap= 0 or 1
     * Space(S)
     *
     * +           R        W        +  
     * |--+--+--+--+--+--+--+--+--+--|--|--
     * 0  1  2  3  4  5  6  7  8  9  0  1
     *
     *  Sw = L - (fL + W - R), W - R < L
     *  Sr = fL + W - R
     *
     *    Sw > 0: writable
     *    Sr > 0: readable
     */

    /* Read Lock */
    pthread_mutex_t RLock;

    /* Write Lock */
    pthread_mutex_t WLock;

    /* Write Offset to the Buffer start */
    shmmap_state_t WOffset;

    /* Read Offset to the Buffer start */
    shmmap_state_t ROffset;

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
int __shmmap_buffer_read_internal (shmmap_buffer_t *shmbuf, ssize_t wrap, ssize_t R, ssize_t W, ssize_t L, int (*nextentry_cb)(const shmmap_entry_t *, void *), void *arg);


NOWARNING_UNUSED(static)
void shmmap_buffer_close (shmmap_buffer_t *shmbuf)
{
    size_t bsize = shmbuf->shmfilesize;
    munmap(shmbuf, bsize);
}


NOWARNING_UNUSED(static)
int shmmap_buffer_delete (const char *shmfilename)
{
    return shm_unlink(shmfilename);
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
        magic = shmmap_decipher_token(shmbuf->cipher, token);
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
 *           if (shmmap_buffer_create(&shmb, shmname, 0666, bsize, &token, 0, 0) == SHMMAP_CREATE_SUCCESS) {
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
 *           if (shmmap_buffer_create(&shmb, shmname, 0666, 0, &token, 0, 0) == SHMMAP_CREATE_SUCCESS) {
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
int shmmap_buffer_create (shmmap_buffer_t **outshmbuf, const char *shmfilename, mode_t filemode, size_t filesize,
    ub8token_t *token,
    ub8token_t (*encipher_cb)(const ub8token_t magic, ub8token_t *token),
    ub8token_t (*decipher_cb)(const ub8token_t magic, ub8token_t *token))
{
    shmmap_buffer_t *shmbuf;

    /* total size in bytes of shmmap file */
    size_t mapfilesize = 0;

    /* aligned shared memory size as Length of ring buffer */
    size_t bufferLength = 0;

    int err, fd, exist = 0;

    if (filesize == SHMMAP_INVALID_STATE) {
        return SHMMAP_CREATE_ERRSIZE;
    }

    fd = shm_open(shmfilename, O_RDWR|O_CREAT|O_EXCL, filemode);
    if (fd == -1 && errno == EEXIST) {
        fd = shm_open(shmfilename, O_RDWR|O_CREAT, filemode);
        exist = 1;
    }
    if (fd == -1) {
        perror("shm_open");
        return SHMMAP_CREATE_ERROPEN;
    }

    if (! exist && ! filesize) {
        /* create a new one shmmap requires filesize */
        close(fd);
        return SHMMAP_CREATE_ERRSIZE;
    }

    if (filesize) {
        bufferLength = SHMMAP_ALIGN_PAGESIZE(filesize);
        mapfilesize = SHMMAP_BUFFER_HDRSIZE + bufferLength;
    }

    if (exist) {
        shmbuf = (shmmap_buffer_t *) mmap(NULL, SHMMAP_BUFFER_HDRSIZE, PROT_READ, MAP_SHARED, fd, 0);
        if (! shmbuf) {
            perror("mmap");
            close(fd);
            return SHMMAP_CREATE_ERRMMAP;
        }

        if (! filesize) {
            bufferLength = shmbuf->Length;
            mapfilesize = shmbuf->shmfilesize;
        }

        if (mapfilesize != shmbuf->shmfilesize || bufferLength != shmbuf->Length) {
            munmap(shmbuf, SHMMAP_BUFFER_HDRSIZE);
            close(fd);
            return SHMMAP_CREATE_ERRSIZE;
        }

        if (! shmmap_verify_token(shmbuf, token, decipher_cb)) {
            munmap(shmbuf, SHMMAP_BUFFER_HDRSIZE);
            close(fd);
            return SHMMAP_CREATE_ERRTOKEN;
        }

        munmap(shmbuf, SHMMAP_BUFFER_HDRSIZE);
    }

    if (! mapfilesize) {
        close(fd);
        return SHMMAP_CREATE_ERRSIZE;
    }

    err = ftruncate(fd, mapfilesize);
    if (err) {
        perror("ftruncate");
        close(fd);
        return SHMMAP_CREATE_ERRTRUNC;
    }

    shmbuf = (shmmap_buffer_t *) mmap(NULL, mapfilesize, PROT_READ|PROT_WRITE, MAP_SHARED, fd, 0);
    if (! shmbuf) {
        perror("mmap");
        close(fd);
        return SHMMAP_CREATE_ERRMMAP;
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
                shmbuf->cipher = shmmap_encipher_token(shmbuf->magic, token);
            }
        }

        shmmap_semaphore_init(&shmbuf->semaphore);

        process_shared_mutex_init(&shmbuf->RLock);
        process_shared_mutex_init(&shmbuf->WLock);

        shmmap_state_init(&shmbuf->WOffset, 0);
        shmmap_state_init(&shmbuf->ROffset, 0);

        shmbuf->Length = bufferLength;
        shmbuf->shmfilesize = mapfilesize;
    }

    if (! shmmap_verify_token(shmbuf, token, decipher_cb)) {
        munmap(shmbuf, mapfilesize);
        close(fd);
        return SHMMAP_CREATE_ERRTOKEN;
    }

    /* create or open success */
    close(fd);

    *outshmbuf = shmbuf;

    return SHMMAP_CREATE_SUCCESS;
}


/**
 * shmmap_buffer_write()
 *   Write chunk data of entry into shmmap ring buffer. 
 *
 * Returns:
 *   SHMMAP_WRITE_SUCCESS(1) - write success
 *   SHMMAP_WRITE_AGAIN(0)   - write again
 *   SHMMAP_WRITE_FATAL(-1)  - fatal write error
 */
NOWARNING_UNUSED(static)
int shmmap_buffer_write (shmmap_buffer_t *shmbuf, const void *chunk, size_t chunksz)
{
    shmmap_entry_t *entry;
    ssize_t Ro, Wo,
        L = (ssize_t)shmbuf->Length,
        AENTSZ = (ssize_t)SHMMAP_ALIGN_ENTRYSIZE(chunksz);

    if (! AENTSZ || AENTSZ == SHMMAP_INVALID_STATE || AENTSZ > (L / SHMMAP_ENTRY_HDRSIZE)) {
        /* fatal error should not occurred! */
    # ifdef SHMMAP_TRACE_PRINT_ON
            printf("(shmmap.h:%d) fatal error: invalid chunksz(=%" PRIu64").\n", __LINE__, chunksz);
    # endif
        return SHMMAP_WRITE_FATAL;
    }

    if (process_shared_mutex_lock(&shmbuf->WLock, 1) == 0) {
        /* Get original ROffset */
        Ro = shmmap_state_get(&shmbuf->ROffset);
        Wo = shmbuf->WOffset.state;

        SHMRINGBUF_RESTORE_STATE(Ro, Wo, L);

    # ifdef SHMMAP_TRACE_PRINT_ON
        if (wrap) {
            printf("(shmmap.h:%d) shmmap_buffer_write(%" PRIu64":%" PRIu64"): W=%" PRId64" R=%" PRId64" L=%" PRId64"\n",
                __LINE__, chunksz, AENTSZ, W, R, L);
        } else {
            printf("(shmmap.h:%d) shmmap_buffer_write(%" PRIu64":%" PRIu64"): R=%" PRId64" W=%" PRId64" L=%" PRId64"\n",
                __LINE__, chunksz, AENTSZ, R, W, L);
        }
    # endif

        /* Sw = L - (wrap*L + W - R) */
        if (L - (wrap*L + W - R) >= AENTSZ) {
            if (wrap) { /* wrap(1): 0 .. W < R < L */
                entry = SHMMAP_ENTRY_CAST(&shmbuf->Buffer[W]);
                entry->size = chunksz;
                memcpy(entry->chunk, chunk, chunksz);

                /* WOffset = Wo + AENTSZ */
                W = SHMRINGBUF_NORMALIZE_OFFSET(Wo + AENTSZ, L);
                shmmap_state_set(&shmbuf->WOffset, W);
            } else {   /* wrap(0): 0 .. R < W < L */
                if (L - W >= AENTSZ) {
                    entry = SHMMAP_ENTRY_CAST(&shmbuf->Buffer[W]);
                    entry->size = chunksz;
                    memcpy(entry->chunk, chunk, chunksz);

                    /* WOffset = Wo + AENTSZ */
                    W = SHMRINGBUF_NORMALIZE_OFFSET(Wo + AENTSZ, L);
                    shmmap_state_set(&shmbuf->WOffset, W);
                } else if (R - 0 >= AENTSZ) {
                    /* clear W slot before wrap W */
                    bzero(&shmbuf->Buffer[W], L - W); 

                    /* wrap W to 0 */
                    entry = SHMMAP_ENTRY_CAST(&shmbuf->Buffer[0]);
                    entry->size = chunksz;
                    memcpy(entry->chunk, chunk, chunksz);

                    /* WOffset = AENTSZ, wrap = 1 */
                    W = AENTSZ + (1 - (int)(Ro/L))*L;

                    shmmap_state_set(&shmbuf->WOffset, W);
                } else {
                    /* no space left to write, expect to call again */
                    process_shared_mutex_unlock(&shmbuf->WLock);
                    return SHMMAP_WRITE_AGAIN;
                }
            }

            process_shared_mutex_unlock(&shmbuf->WLock);
            return SHMMAP_WRITE_SUCCESS;
        }

        /* no space left to write */
        process_shared_mutex_unlock(&shmbuf->WLock);
        return SHMMAP_WRITE_AGAIN;
    }

    /* lock fail to write, expect to call again */
    return SHMMAP_WRITE_AGAIN;
}


/**
 * shmmap_buffer_read_copy()
 *   Copy entry from shmmap ringbuffer into rdbuf.
 *
 * returns:
 *   SHMMAP_READ_AGAIN(0)       - read again
 *   SHMMAP_READ_FATAL(-1)      - fatal read error
 *
 *   ret > 0 and ret <= rdbufsz - read success
 *   ret > rdbufsz              - no read for insufficient buffer
 */
NOWARNING_UNUSED(static)
size_t shmmap_buffer_read_copy (shmmap_buffer_t *shmbuf, char *rdbuf, size_t rdbufsz)
{
    shmmap_entry_t *entry;

    ssize_t Ro, Wo, entsize, AENTSZ, 
            L = (ssize_t)shmbuf->Length,
            HENTSZ = (ssize_t)SHMMAP_ALIGN_ENTRYSIZE(0);

    if (process_shared_mutex_lock(&shmbuf->RLock, 1) == 0) {
        Wo = shmmap_state_get(&shmbuf->WOffset);
        Ro = shmbuf->ROffset.state;

        SHMRINGBUF_RESTORE_STATE(Ro, Wo, L);

    # ifdef SHMMAP_TRACE_PRINT_ON
        if (wrap) {
            printf("(shmmap.h:%d) shmmap_buffer_read_copy(): W=%" PRId64" R=%" PRId64" L=%" PRId64"\n", __LINE__, W, R, L);
        } else {
            printf("(shmmap.h:%d) shmmap_buffer_read_copy(): R=%" PRId64" W=%" PRId64" L=%" PRId64"\n", __LINE__, R, W, L);
        }
    # endif

        /* Sr = f*L + W - R */
        if (wrap*L + W - R > HENTSZ) {
            if (wrap) {  /* wrap(1): 0 .. W < R < L */
                if (L - R > HENTSZ) {
                    entry = SHMMAP_ENTRY_CAST(&shmbuf->Buffer[R]);
                    if (entry->size) {
                        AENTSZ = SHMMAP_ALIGN_ENTRYSIZE(entry->size);

                        if (L - R >= AENTSZ) {
                            entsize = entry->size;
                            if (entsize > rdbufsz) {
                                /* read buf is insufficient for entry.
                                 * read nothing if returned entsize > rdbufsz */
                                process_shared_mutex_unlock(&shmbuf->RLock);
                                return (size_t)entsize;
                            }

                            /* read entry chunk into rdbuf ok */
                            memcpy(rdbuf, entry->chunk, entsize);

                            R = SHMRINGBUF_NORMALIZE_OFFSET(Ro + AENTSZ, L);
                            shmmap_state_set(&shmbuf->ROffset, R);

                            /* read success if returned entsize <= rdbufsz */
                            process_shared_mutex_unlock(&shmbuf->RLock);
                            return (size_t)entsize;
                        }

                        printf("(shmmap.h:%d) SHOULD NEVER RUN TO THIS! fatal bug.\n", __LINE__);
                        process_shared_mutex_unlock(&shmbuf->RLock);
                        return SHMMAP_READ_FATAL;
                    } else {
                        /* reset ROffset to 0 (set wrap = 0) */
                        shmmap_state_set(&shmbuf->ROffset, (Wo/L) * L);

                        /* expect to read again */
                        process_shared_mutex_unlock(&shmbuf->RLock);
                        return shmmap_buffer_read_copy(shmbuf, rdbuf, rdbufsz);
                    }
                } else if (W - 0 > HENTSZ) { 
                    /* reset ROffset to 0 */
                    entry = SHMMAP_ENTRY_CAST(&shmbuf->Buffer[0]);
                    if (entry->size) {
                        AENTSZ = SHMMAP_ALIGN_ENTRYSIZE(entry->size);

                        if (W - 0 >= AENTSZ) {
                            entsize = entry->size;
                            if (entsize > rdbufsz) {
                                /* read buf is insufficient for entry.
                                 * read nothing if returned entsize > rdbufsz */
                                process_shared_mutex_unlock(&shmbuf->RLock);
                                return (size_t)entsize;
                            }

                            /* read entry chunk into rdbuf ok */
                            memcpy(rdbuf, entry->chunk, entsize);

                            /* ROffset = AENTSZ, wrap = 0 */
                            shmmap_state_set(&shmbuf->ROffset, AENTSZ + (Wo/L)*L);

                            /* read success if returned entsize <= rdbufsz */
                            process_shared_mutex_unlock(&shmbuf->RLock);
                            return (size_t)entsize;
                        }
                    }
                    
                    printf("(shmmap.h:%d) SHOULD NEVER RUN TO THIS! fatal bug.\n", __LINE__);
                    process_shared_mutex_unlock(&shmbuf->RLock);
                    return SHMMAP_READ_FATAL;
                }
            } else {  /* wrap(0): 0 .. R < W < L */
                entry = SHMMAP_ENTRY_CAST(&shmbuf->Buffer[R]);
                if (entry->size) {
                    AENTSZ = SHMMAP_ALIGN_ENTRYSIZE(entry->size);

                    if (W - R >= AENTSZ) {
                        entsize = entry->size;
                        if (entsize > rdbufsz) {
                            /* read buf is insufficient for entry.
                             * read nothing if returned entsize > rdbufsz
                             */
                            process_shared_mutex_unlock(&shmbuf->RLock);
                            return (size_t)entsize;
                        }

                        /* read entry chunk into rdbuf ok */
                        memcpy(rdbuf, entry->chunk, entsize);

                        R = SHMRINGBUF_NORMALIZE_OFFSET(Ro + AENTSZ, L);
                        shmmap_state_set(&shmbuf->ROffset, R);

                        /* read success if returned entsize <= rdbufsz */
                        process_shared_mutex_unlock(&shmbuf->RLock);
                        return (size_t)entsize;
                    }
                }

                printf("(shmmap.h:%d) SHOULD NEVER RUN TO THIS! fatal bug.\n", __LINE__);
                process_shared_mutex_unlock(&shmbuf->RLock);
                return SHMMAP_READ_FATAL;
            }
        }

        /* no entry to read, retry again */
        process_shared_mutex_unlock(&shmbuf->RLock);
        return SHMMAP_READ_AGAIN;
    }

    /* read locked fail, retry again */
    return SHMMAP_READ_AGAIN;
}


/**
 * shmmap_buffer_read_next()
 *   Read next one entry from shmmap ring buffer into callback (no copy data).
 *
 * params:
 *   nextentry_cb() - Callback implemented by caller should ONLY
 *                     return SHMMAP_READ_NEXT(1) or SHMMAP_READ_AGAIN(0)
 *                    DO NOT change and members of entry in nextentry_cb().
 * returns:
 *   SHMMAP_READ_NEXT(1)    - read for next one
 *   SHMMAP_READ_AGAIN(0)   - read current one again
 *   SHMMAP_READ_FATAL(-1)  - fatal read error
 */
NOWARNING_UNUSED(static)
int shmmap_buffer_read_next (shmmap_buffer_t *shmbuf, int (*nextentry_cb)(const shmmap_entry_t *entry, void *arg), void *arg)
{
    int ret = SHMMAP_READ_AGAIN;

    if (process_shared_mutex_lock(&shmbuf->RLock, 1) == 0) {
        ssize_t Wo = shmmap_state_get(&shmbuf->WOffset);
        ssize_t Ro = shmbuf->ROffset.state;
        ssize_t wrap = SHMRINGBUF_RESTORE_WRAP(Ro, Wo, shmbuf->Length);

        ret = __shmmap_buffer_read_internal(shmbuf, wrap, Ro, Wo, (ssize_t)shmbuf->Length, nextentry_cb, arg);

        process_shared_mutex_unlock(&shmbuf->RLock);
    }

    return ret;
}



/**
 * shmmap_buffer_read_next_batch()
 *   Read next batch entries from shmmap ring buffer into callback (no copy data).
 */
NOWARNING_UNUSED(static)
int shmmap_buffer_read_next_batch (shmmap_buffer_t *shmbuf, int (*nextentry_cb)(const shmmap_entry_t *entry, void *arg), void *arg, int batch)
{
    int num = 0, ret = SHMMAP_READ_AGAIN;

    if (process_shared_mutex_lock(&shmbuf->RLock, 1) == 0) {
        ssize_t wrap, Wo, Ro;

        while (batch-- > 0) {
            Wo = shmmap_state_get(&shmbuf->WOffset);
            Ro = shmbuf->ROffset.state;
            wrap = SHMRINGBUF_RESTORE_WRAP(Ro, Wo, shmbuf->Length);

            ret = __shmmap_buffer_read_internal(shmbuf, wrap, Ro, Wo, (ssize_t)shmbuf->Length, nextentry_cb, arg);
            if (ret == SHMMAP_READ_NEXT) {
                num++;
                continue;
            }

            if (ret == SHMMAP_READ_AGAIN) {
                /* stop current batch */
                break;
            }

            if (ret == SHMMAP_READ_FATAL) {
                num = (int) SHMMAP_READ_FATAL;
                break;
            }

            /* SHOULD NEVER RUN TO THIS ! */
        }

        process_shared_mutex_unlock(&shmbuf->RLock);
    }

    return num;
}


NOWARNING_UNUSED(static)
int shmmap_buffer_post (shmmap_buffer_t *shmbuf, size_t timeout_us)
{
    return shmmap_semaphore_post(&shmbuf->semaphore, timeout_us);
}


NOWARNING_UNUSED(static)
int shmmap_buffer_wait (shmmap_buffer_t *shmbuf, size_t timeout_us)
{
    return shmmap_semaphore_wait(&shmbuf->semaphore, timeout_us);
}


/**
 * shmmap_buffer_force_unlock()
 *   Force unlock state lock. statelock can be one or combination of below:
 *
 *     SHMMAP_READSTATE_LOCK
 *     SHMMAP_WRITESTATE_LOCK
 * NOTES:
 *   Make sure prior to force unlocking statelock, all the applications using
 *    the same shmmap file should be paused!
 */
#define SHMMAP_READSTATE_LOCK    1
#define SHMMAP_WRITESTATE_LOCK   2

NOWARNING_UNUSED(static)
void shmmap_buffer_force_unlock (shmmap_buffer_t *shmbuf, int statelock)
{
    if (SHMMAP_READSTATE_LOCK & statelock) {
        process_shared_mutex_unlock(&shmbuf->RLock);
    }

    if (SHMMAP_WRITESTATE_LOCK & statelock) {
        process_shared_mutex_unlock(&shmbuf->WLock);
    }
}


/**
 * __shmmap_buffer_read_internal()
 *   Private function. do not call it in your code !!
 */
int __shmmap_buffer_read_internal (shmmap_buffer_t *shmbuf, ssize_t wrap, ssize_t Ro, ssize_t Wo, ssize_t L, int (*nextentry_cb)(const shmmap_entry_t *, void *), void *arg)
{
    shmmap_entry_t *entry;

    ssize_t AENTSZ,
            HENTSZ = (ssize_t)SHMMAP_ALIGN_ENTRYSIZE(0);

    ssize_t R = Ro % L;
    ssize_t W = Wo % L;

# ifdef SHMMAP_TRACE_PRINT_ON
    if (wrap) {
        printf("(shmmap.h:%d) shmmap_buffer_read_next(): W=%" PRId64" R=%" PRId64" L=%" PRId64"\n", __LINE__, W, R, L);
    } else {
        printf("(shmmap.h:%d) shmmap_buffer_read_next(): R=%" PRId64" W=%" PRId64" L=%" PRId64"\n", __LINE__, R, W, L);
    }
# endif

    /* Sr = f*L + W - R */
    if (wrap*L + W - R > HENTSZ) {
        if (wrap) {  /* wrap(1): 0 .. W < R < L */
            if (L - R > HENTSZ) {
                entry = SHMMAP_ENTRY_CAST(&shmbuf->Buffer[R]);
                if (entry->size) {
                    AENTSZ = SHMMAP_ALIGN_ENTRYSIZE(entry->size);

                    if (L - R >= AENTSZ) {
                        if (nextentry_cb(entry, arg)) {
                            /* read success and set ROffset to next entry */
                            R = SHMRINGBUF_NORMALIZE_OFFSET(Ro + AENTSZ, L);
                            shmmap_state_set(&shmbuf->ROffset, R);

                            return SHMMAP_READ_NEXT;
                        } else {
                            /* read paused by caller */
                            return SHMMAP_READ_AGAIN;
                        }
                    }

                    printf("(shmmap.h:%d) SHOULD NEVER RUN TO THIS! fatal bug.\n", __LINE__);
                    return SHMMAP_READ_FATAL;
                } else {
                    /* reset ROffset to 0 (set wrap = 0) */
                    shmmap_state_set(&shmbuf->ROffset, (Wo/L) * L);

                    return shmmap_buffer_read_next(shmbuf, nextentry_cb, arg);
                }
            } else if (W - 0 > HENTSZ) { 
                /* reset ROffset to 0 */
                entry = SHMMAP_ENTRY_CAST(&shmbuf->Buffer[0]);
                if (entry->size) {
                    AENTSZ = SHMMAP_ALIGN_ENTRYSIZE(entry->size);

                    if (W - 0 >= AENTSZ) {
                        if (nextentry_cb(entry, arg)) {
                            /* read success and set ROffset to next entry */
                            shmmap_state_set(&shmbuf->ROffset, AENTSZ + (Wo/L)*L);

                            return SHMMAP_READ_NEXT;
                        } else {
                            /* read paused by caller */
                            return SHMMAP_READ_AGAIN;
                        }
                    }
                }

                printf("(shmmap.h:%d) SHOULD NEVER RUN TO THIS! fatal bug.\n", __LINE__);
                return SHMMAP_READ_FATAL;
            }
        } else {  /* wrap(0): 0 .. R < W < L */
            entry = SHMMAP_ENTRY_CAST(&shmbuf->Buffer[R]);
            if (entry->size) {
                AENTSZ = SHMMAP_ALIGN_ENTRYSIZE(entry->size);

                if (W - R >= AENTSZ) {
                    if (nextentry_cb(entry, arg)) {
                        /* read success and set ROffset to next entry */
                        R = SHMRINGBUF_NORMALIZE_OFFSET(Ro + AENTSZ, L);
                        shmmap_state_set(&shmbuf->ROffset, R);

                        return SHMMAP_READ_NEXT;
                    } else {
                        /* read paused by caller */
                        return SHMMAP_READ_AGAIN;
                    }
                }
            }

            printf("(shmmap.h:%d) SHOULD NEVER RUN TO THIS! fatal bug.\n", __LINE__);
            return SHMMAP_READ_FATAL;
        }
    }

    /* no entry to read, retry again */
    return SHMMAP_READ_AGAIN;
}

#ifdef __cplusplus
}
#endif
#endif /* SHMMAP_H__ */