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
** @file ringbufst.h
**   ring buffer with static memory allocation.
**
** see also:
**   ringbuf.h (a ring buffer with dynamic memory allocation)
**
** @author     Liang Zhang <350137278@qq.com>
** @version 1.0.4
** @create     2020-06-24 12:46:50
** @update     2020-06-24 14:25:08
**
*/
#ifndef RINGBUFST_H__
#define RINGBUFST_H__

#if defined(__cplusplus)
extern "C"
{
#endif

#include "basetype.h"
#include "uatomic.h"
#include "memapi.h"

#ifdef __WINDOWS__
# define INT_CAST_TO_LONG(s)  ((LONG)(s))
#else
# define INT_CAST_TO_LONG(s)  (s)
#endif


#define RINGBUFST_LENGTH_MAX     0x01FFFF

/* must > 1 */
#define RINGBUFST_LENGTH_MIN     2

/**
 * Below definitions SHOULD NOT be changed!
 */
#define RINGBUFST_PAGE_SIZE        ((size_t)4096)

#define RINGBUFST_INVALID_STATE    ((ssize_t)(-1))

#define RINGBUFST_ENTRY_HDRSIZE    (sizeof(ringbuf_entry_st))

#define RINGBUFST_ALIGN_PAGESIZE(fsz)    \
            memapi_align_bsize((fsz), RINGBUFST_PAGE_SIZE)

#define RINGBUFST_ALIGN_ENTRYSIZE(chunksz)    \
            memapi_align_bsize((chunksz + RINGBUFST_ENTRY_HDRSIZE), RINGBUFST_ENTRY_HDRSIZE)

#define RINGBUFST_ENTRY_CAST(p)  ((ringbuf_entry_st *)(p))


/**
 * The layout of memory for any one entry in ringbufst.
 */
typedef struct _ringbuf_entry_st
{
    size_t size;
    char chunk[0];
} ringbuf_entry_st;


/**
 * static memory layout of ring buffer.
 * see also:
 *   ringbuf.h
 */
typedef struct _ring_buffer_st
{
    /* Read Lock */
    uatomic_int RLock;

    /* Write Lock */
    uatomic_int WLock;

    /* Write Offset to the Buffer start */
    uatomic_int WOffset;

    /* Read Offset to the Buffer start */
    uatomic_int ROffset;

    /* Length of ring Buffer: total size in bytes */
    size_t Length;

    /* ring buffer static memory with Length */
    char Buffer[0];
} ring_buffer_st;


#define RINGBUFST_RESTORE_WRAP(Ro, Wo, L)  \
    ((ssize_t)((Ro)/(L) == (Wo)/(L) ? 0 : 1))


#define RINGBUFST_RESTORE_STATE(Ro, Wo, L)  \
    ssize_t wrap = RINGBUFST_RESTORE_WRAP(Ro, Wo, L); \
    ssize_t R = (ssize_t)((Ro) % (L)); \
    ssize_t W = (ssize_t)((Wo) % (L))


#define RINGBUFST_NORMALIZE_OFFSET(Ao, L)  \
    ((((Ao)/(L))%2)*L + (Ao)%(L))


/**
 * public interface
 */

static ring_buffer_st * ringbufst_init (int length, int eltsizemax)
{
    ring_buffer_st *rbst;
    size_t cbLength;

    if (length < RINGBUFST_LENGTH_MIN) {
        length = RINGBUFST_LENGTH_MIN;
    }
    if (length > RINGBUFST_LENGTH_MAX) {
        length = RINGBUFST_LENGTH_MAX;
    }

    /* new and initialize read and write index by 0 */
    cbLength = RINGBUFST_ALIGN_PAGESIZE(eltsizemax * length);

    rbst = (ring_buffer_st *) mem_alloc_zero(1, sizeof(*rbst) + cbLength);

    rbst->Length = cbLength;

    return rbst;
}


static void ringbufst_uninit (ring_buffer_st *rbst)
{
    uatomic_int_set(&rbst->RLock, 1);
    uatomic_int_set(&rbst->WLock, 1);
    mem_free(rbst);
}


static int ringbufst_write (ring_buffer_st *rbst, size_t chunksz, void(*write_cb)(char *, size_t, void *), void *arg)
{
    ringbuf_entry_st *entry;

    ssize_t Ro, Wo,
        L = (ssize_t) rbst->Length,
        AENTSZ = (ssize_t) RINGBUFST_ALIGN_ENTRYSIZE(chunksz);

    if (! AENTSZ || AENTSZ == RINGBUFST_INVALID_STATE || AENTSZ > (ssize_t) (L / RINGBUFST_ENTRY_HDRSIZE)) {
        /* fatal error should not occurred! */
        return (-1);
    }

    if (! uatomic_int_comp_exch(&rbst->WLock, 0, 1)) {
        /* Get original ROffset */
        Ro = uatomic_int_get(&rbst->ROffset);
        Wo = rbst->WOffset;

        RINGBUFST_RESTORE_STATE(Ro, Wo, L);

        /* Sw = L - (wrap*L + W - R) */
        if (L - (wrap*L + W - R) >= AENTSZ) {
            if (wrap) { /* wrap(1): 0 .. W < R < L */
                entry = RINGBUFST_ENTRY_CAST(&rbst->Buffer[W]);
                entry->size = chunksz;

                write_cb(entry->chunk, entry->size, arg);

                /* WOffset = Wo + AENTSZ */
                W = RINGBUFST_NORMALIZE_OFFSET(Wo + AENTSZ, L);
                uatomic_int_set(&rbst->WOffset, INT_CAST_TO_LONG(W));
            } else {   /* wrap(0): 0 .. R < W < L */
                if (L - W >= AENTSZ) {
                    entry = RINGBUFST_ENTRY_CAST(&rbst->Buffer[W]);
                    entry->size = chunksz;

                    write_cb(entry->chunk, entry->size, arg);

                    /* WOffset = Wo + AENTSZ */
                    W = RINGBUFST_NORMALIZE_OFFSET(Wo + AENTSZ, L);
                    uatomic_int_set(&rbst->WOffset, INT_CAST_TO_LONG(W));
                } else if (R - 0 >= AENTSZ) {
                    /* clear W slot before wrap W */
                    bzero(&rbst->Buffer[W], L - W);

                    /* wrap W to 0 */
                    entry = RINGBUFST_ENTRY_CAST(&rbst->Buffer[0]);
                    entry->size = chunksz;

                    write_cb(entry->chunk, entry->size, arg);

                    /* WOffset = AENTSZ, wrap = 1 */
                    W = AENTSZ + (1 - (int)(Ro/L))*L;

                    uatomic_int_set(&rbst->WOffset, INT_CAST_TO_LONG(W));
                } else {
                    /* no space left to write. expect to call again(0) */
                    uatomic_int_zero(&rbst->WLock);
                    return 0;
                }
            }

            /* write success */
            uatomic_int_zero(&rbst->WLock);
            return 1;
        }

        /* no space left to write. expect to call again(0) */
        uatomic_int_zero(&rbst->WLock);
        return 0;
    }

    /* lock fail to write, expect to call again(0) */
    return 0;
}


static size_t ringbufst_read_copy (ring_buffer_st *rbst, char *rdbuf, size_t rdbufsz)
{
    ringbuf_entry_st *entry;

    ssize_t Ro, Wo, entsize, AENTSZ,
            L = (ssize_t) rbst->Length,
            HENTSZ = (ssize_t)RINGBUFST_ALIGN_ENTRYSIZE(0);

    if (! uatomic_int_comp_exch(&rbst->RLock, 0, 1)) {
        Wo = uatomic_int_get(&rbst->WOffset);
        Ro = rbst->ROffset;

        RINGBUFST_RESTORE_STATE(Ro, Wo, L);

        /* Sr = f*L + W - R */
        if (wrap*L + W - R > HENTSZ) {
            if (wrap) {  /* wrap(1): 0 .. W < R < L */
                if (L - R > HENTSZ) {
                    entry = RINGBUFST_ENTRY_CAST(&rbst->Buffer[R]);
                    if (entry->size) {
                        AENTSZ = RINGBUFST_ALIGN_ENTRYSIZE(entry->size);

                        if (L - R >= AENTSZ) {
                            entsize = entry->size;
                            if (entsize > (ssize_t) rdbufsz) {
                                /* read buf is insufficient for entry.
                                 * read nothing if returned entsize > rdbufsz */
                                uatomic_int_zero(&rbst->RLock);
                                return (size_t)entsize;
                            }

                            /* read entry chunk into rdbuf ok */
                            memcpy(rdbuf, entry->chunk, entsize);

                            R = RINGBUFST_NORMALIZE_OFFSET(Ro + AENTSZ, L);
                            uatomic_int_set(&rbst->ROffset, INT_CAST_TO_LONG(R));

                            /* read success if returned entsize <= rdbufsz */
                            uatomic_int_zero(&rbst->RLock);
                            return (size_t)entsize;
                        }

                        uatomic_int_zero(&rbst->RLock);
                        return (-1);
                    } else {
                        /* reset ROffset to 0 (set wrap = 0) */
                        uatomic_int_set(&rbst->ROffset, INT_CAST_TO_LONG((Wo/L) * L));

                        /* expect to read again */
                        uatomic_int_zero(&rbst->RLock);
                        return ringbufst_read_copy(rbst, rdbuf, rdbufsz);
                    }
                } else if (W - 0 > HENTSZ) {
                    /* reset ROffset to 0 */
                    entry = RINGBUFST_ENTRY_CAST(&rbst->Buffer[0]);
                    if (entry->size) {
                        AENTSZ = RINGBUFST_ALIGN_ENTRYSIZE(entry->size);

                        if (W - 0 >= AENTSZ) {
                            entsize = entry->size;
                            if (entsize > (ssize_t) rdbufsz) {
                                /* read buf is insufficient for entry.
                                 * read nothing if returned entsize > rdbufsz */
                                uatomic_int_zero(&rbst->RLock);
                                return (size_t)entsize;
                            }

                            /* read entry chunk into rdbuf ok */
                            memcpy(rdbuf, entry->chunk, entsize);

                            /* ROffset = AENTSZ, wrap = 0 */
                            uatomic_int_set(&rbst->ROffset, INT_CAST_TO_LONG(AENTSZ + (Wo/L)*L));

                            /* read success if returned entsize <= rdbufsz */
                            uatomic_int_zero(&rbst->RLock);
                            return (size_t)entsize;
                        }
                    }

                    uatomic_int_zero(&rbst->RLock);
                    return (-1);
                }
            } else {  /* wrap(0): 0 .. R < W < L */
                entry = RINGBUFST_ENTRY_CAST(&rbst->Buffer[R]);
                if (entry->size) {
                    AENTSZ = RINGBUFST_ALIGN_ENTRYSIZE(entry->size);

                    if (W - R >= AENTSZ) {
                        entsize = entry->size;
                        if (entsize > (ssize_t) rdbufsz) {
                            /* read buf is insufficient for entry.
                             * read nothing if returned entsize > rdbufsz
                             */
                            uatomic_int_zero(&rbst->RLock);
                            return (size_t)entsize;
                        }

                        /* read entry chunk into rdbuf ok */
                        memcpy(rdbuf, entry->chunk, entsize);

                        R = RINGBUFST_NORMALIZE_OFFSET(Ro + AENTSZ, L);
                        uatomic_int_set(&rbst->ROffset, INT_CAST_TO_LONG(R));

                        /* read success if returned entsize <= rdbufsz */
                        uatomic_int_zero(&rbst->RLock);
                        return (size_t) entsize;
                    }
                }

                uatomic_int_zero(&rbst->RLock);
                return (-1);
            }
        }

        /* no entry to read, retry again(0) */
        uatomic_int_zero(&rbst->RLock);
        return 0;
    }

    /* read locked fail, retry again(0) */
    return 0;
}

static int __ringbufst_read_internal (ring_buffer_st *rbst, ssize_t wrap, ssize_t R, ssize_t W, ssize_t L, int (*nextentry_cb)(const ringbuf_entry_st *, void *), void *arg);

static int ringbufst_read_next (ring_buffer_st *rbst, int (*nextentry_cb)(const ringbuf_entry_st *entry, void *arg), void *arg)
{
    /* again(0) */
    int ret = 0;

    if (! uatomic_int_comp_exch(&rbst->RLock, 0, 1)) {
        ssize_t Wo = uatomic_int_get(&rbst->WOffset);
        ssize_t Ro = rbst->ROffset;
        ssize_t wrap = RINGBUFST_RESTORE_WRAP(Ro, Wo, rbst->Length);

        ret = __ringbufst_read_internal(rbst, wrap, Ro, Wo, (ssize_t)rbst->Length, nextentry_cb, arg);

        uatomic_int_zero(&rbst->RLock);
    }

    return ret;
}


static int ringbufst_read_next_batch (ring_buffer_st *rbst, int (*nextentry_cb)(const ringbuf_entry_st *, void *), void *arg, int batch)
{
    int num = 0, ret = 0;

    if (! uatomic_int_comp_exch(&rbst->RLock, 0, 1)) {
        ssize_t wrap, Wo, Ro;

        while (batch-- > 0) {
            Wo = uatomic_int_get(&rbst->WOffset);
            Ro = rbst->ROffset;
            wrap = RINGBUFST_RESTORE_WRAP(Ro, Wo, rbst->Length);

            ret = __ringbufst_read_internal(rbst, wrap, Ro, Wo, (ssize_t)rbst->Length, nextentry_cb, arg);
            if (ret == 1) {
                num++;
                continue;
            }

            if (ret == 0) {
                /* stop current batch */
                break;
            }

            if (ret == -1) {
                num = (-1);
                break;
            }

            /* SHOULD NEVER RUN TO THIS ! */
        }

        uatomic_int_zero(&rbst->RLock);
    }

    return num;
}


int __ringbufst_read_internal (ring_buffer_st *rbst, ssize_t wrap, ssize_t Ro, ssize_t Wo, ssize_t L, int (*nextentry_cb)(const ringbuf_entry_st *, void *), void *arg)
{
    ringbuf_entry_st *entry;

    ssize_t AENTSZ,
            HENTSZ = (ssize_t)RINGBUFST_ALIGN_ENTRYSIZE(0);

    ssize_t R = Ro % L;
    ssize_t W = Wo % L;

    /* Sr = f*L + W - R */
    if (wrap*L + W - R > HENTSZ) {
        if (wrap) {  /* wrap(1): 0 .. W < R < L */
            if (L - R > HENTSZ) {
                entry = RINGBUFST_ENTRY_CAST(&rbst->Buffer[R]);
                if (entry->size) {
                    AENTSZ = RINGBUFST_ALIGN_ENTRYSIZE(entry->size);

                    if (L - R >= AENTSZ) {
                        if (nextentry_cb(entry, arg)) {
                            /* read success and set ROffset to next entry */
                            R = RINGBUFST_NORMALIZE_OFFSET(Ro + AENTSZ, L);
                            uatomic_int_set(&rbst->ROffset, INT_CAST_TO_LONG(R));

                            return 1;
                        } else {
                            /* no entry to read, retry again(0) */
                            return 0;
                        }
                    }

                    /* SHOULD NEVER RUN TO THIS! */
                    return (-1);
                } else {
                    /* reset ROffset to 0 (set wrap = 0) */
                    uatomic_int_set(&rbst->ROffset, INT_CAST_TO_LONG((Wo/L) * L));

                    return ringbufst_read_next(rbst, nextentry_cb, arg);
                }
            } else if (W - 0 > HENTSZ) {
                /* reset ROffset to 0 */
                entry = RINGBUFST_ENTRY_CAST(&rbst->Buffer[0]);
                if (entry->size) {
                    AENTSZ = RINGBUFST_ALIGN_ENTRYSIZE(entry->size);

                    if (W - 0 >= AENTSZ) {
                        if (nextentry_cb(entry, arg)) {
                            /* read success and set ROffset to next entry */
                            uatomic_int_set(&rbst->ROffset, INT_CAST_TO_LONG(AENTSZ + (Wo/L)*L));

                            return 1;
                        } else {
                            /* read paused by caller, retry again(0) */
                            return 0;
                        }
                    }
                }

                /* SHOULD NEVER RUN TO THIS! */
                return (-1);
            }
        } else {  /* wrap(0): 0 .. R < W < L */
            entry = RINGBUFST_ENTRY_CAST(&rbst->Buffer[R]);
            if (entry->size) {
                AENTSZ = RINGBUFST_ALIGN_ENTRYSIZE(entry->size);

                if (W - R >= AENTSZ) {
                    if (nextentry_cb(entry, arg)) {
                        /* read success and set ROffset to next entry */
                        R = RINGBUFST_NORMALIZE_OFFSET(Ro + AENTSZ, L);
                        uatomic_int_set(&rbst->ROffset, INT_CAST_TO_LONG(R));

                        return 1;
                    } else {
                        /* read paused by caller, retry again(0) */
                        return 0;
                    }
                }
            }

            /* SHOULD NEVER RUN TO THIS! */
            return (-1);
        }
    }

    /* no entry to read, retry again(0) */
    return 0;
}

#ifdef __cplusplus
}
#endif
#endif /* RINGBUFST_H__ */