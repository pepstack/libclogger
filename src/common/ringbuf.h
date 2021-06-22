/***********************************************************************
 * Copyright (c) 2008-2080 pepstack.com, 350137278@qq.com
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
 * @filename   ringbuf.h
 *  Multi-threads Safety Ring Buffer.
 *
 *  This code has been tested OK on 20 threads!
 *
 * @author     Liang Zhang <350137278@qq.com>
 * @version    1.0.0
 * @create     2019-12-14 12:46:50
 * @update     2020-06-30 11:32:10
 */

/**************************************************************
 * Length(L) = 10, Read(R), Write(W), wrap factor=0,1
 * Space(S)
 *
 * +           R        W        +                             +
 * |--+--+--+--+--+--+--+--+--+--|--+--+--+--+--+--+--+--+--+--|--+--+--+--
 * 0  1  2  3  4  5  6  7  8  9  0  1  2  3  4  5  6  7  8  9  0  1  2  3
 *
 *  S = L - (fL + W - R), W - R < L
 *  D = fL + W - R
 *
 *    S > 0: writable
 *    D > 0: readable
 *************************************************************/

/**************************************************************
 *
 *                 RW
 *  wrap=0      |----------|----------|---
 *                    L          L
 *                     R     W
 *  wrap=1      |----------|----------|---
 *
 *                             RW
 *  wrap=0      |----------|----------|---
 *
 *               W                 R
 *  wrap=1      |----------|----------|---
 *
 *                  RW
 *  wrap=0      |----------|----------|---
 *
 *  wrap(R, W, L) => ((R/L == W/L)? 0 : 1)
 *
 *  A = W + 1 (or A = R + 1)
 *
 *  W1 = (( A/L ) %2 ) * L + A % L
 *************************************************************/

#ifndef _RINGBUF_H_
#define _RINGBUF_H_

#if defined(__cplusplus)
extern "C"
{
#endif

#include "uatomic.h"
#include "memapi.h"

#define RINGBUF_LENGTH_MAX     0x01FFFF

/* must > 1 */
#define RINGBUF_LENGTH_MIN     2


#define RINGBUF_RESTORE_STATE(Ro, Wo, L)  \
    int wrap = ((int)((Ro)/(L) == (Wo)/(L) ? 0 : 1)); \
    int R = (Ro) % (L); \
    int W = (Wo) % (L)


#define RINGBUF_NORMALIZE_OFFSET(Ao, L)   \
    ((((Ao)/(L))%2)*L + (Ao)%(L))


#define ringbuf_elt_free(elt)  free(elt)

typedef void (*ringbuf_elt_free_cb)(void *);


typedef struct
{
    ringbuf_elt_free_cb free_cb;
    size_t size;
    char data[1];
} ringbuf_elt_t, *ringbuf_eltp;


static char * ringbuf_elt_new (size_t datasize, ringbuf_elt_free_cb free_cb, ringbuf_elt_t **eltp)
{
    ringbuf_elt_t *elt = (ringbuf_elt_t *) mem_alloc_zero(1, sizeof(*elt) + datasize);
    if (! elt) {
        /* out memory */
        exit(EXIT_FAILURE);
    }

    elt->free_cb = free_cb;
    elt->size = datasize;

    *eltp = elt;
    return (char *) elt->data;
}


typedef struct
{
    /* MT-safety Read Lock */
    uatomic_int RLock;

    /* MT-safety Write Lock */
    uatomic_int WLock;

    /* Write (push) index: 0, L-1 */
    uatomic_int W;

    /* Read (pop) index: 0, L-1 */
    uatomic_int R;

    /* Length of ring buffer */
    int L;

    /* Buffer */
    ringbuf_elt_t *B[0];
} ringbuf_t;


/**
 * public interface
 */

static ringbuf_t * ringbuf_init(int length)
{
    ringbuf_t *rb;

    if (length < RINGBUF_LENGTH_MIN) {
        length = RINGBUF_LENGTH_MIN;
    }
    if (length > RINGBUF_LENGTH_MAX) {
        length = RINGBUF_LENGTH_MAX;
    }

    /* new and initialize read and write index by 0 */
    rb = (ringbuf_t *) mem_alloc_zero(1, sizeof(*rb) + sizeof(ringbuf_elt_t*) * length);
    rb->L = length;
    return rb;
}


static void ringbuf_uninit(ringbuf_t *rb)
{
    uatomic_int_set(&rb->RLock, 1);
    uatomic_int_set(&rb->WLock, 1);

    ringbuf_elt_free_cb elt_free;
    int i = rb->L;
    while (i-- > 0) {
        ringbuf_elt_t *elt = rb->B[i];
        if (elt) {
            rb->B[i] = NULL;

            elt_free = elt->free_cb;
            if (elt_free) {
                elt_free(elt);
            }
        }
    }
    mem_free(rb);
}


static int ringbuf_push (ringbuf_t *rb, ringbuf_elt_t *elt)
{
    if (! uatomic_int_comp_exch(&rb->WLock, 0, 1)) {
        /* copy constant of length */
        int L = rb->L;

        /* original values of R, W */
        int Ro = uatomic_int_get(&rb->R);

        /* MUST get W after R */
        int Wo = rb->W;

        RINGBUF_RESTORE_STATE(Ro, Wo, L);

        /* Sw = L - (f*L+W - R) */
        if (L + R - wrap*L - W > 0) {
            /* writable: Sw > 0 */
            rb->B[W] = elt;

            /* to next Write offset */
            ++Wo;

            /* W = [0, 2L) */
            W = RINGBUF_NORMALIZE_OFFSET(Wo, L);

            uatomic_int_set(&rb->W, W);

            /* push success */
            uatomic_int_zero(&rb->WLock);
            return 1;
        }

        uatomic_int_zero(&rb->WLock);
    }

    /* push failed */
    return 0;
}


static int ringbuf_pop (ringbuf_t *rb, ringbuf_elt_t **eltp)
{
    if (! uatomic_int_comp_exch(&rb->RLock, 0, 1)) {
        int L = rb->L;

        /* original values of W, R */
        int Wo = uatomic_int_get(&rb->W);

        /* MUST get R after W */
        int Ro = rb->R;

        RINGBUF_RESTORE_STATE(Ro, Wo, L);

        /* Sr = f*L + W - R */
        if (wrap*L + W - R > 0) {
            /* readable: Sr > 0 */
            *eltp = rb->B[R];

            /* must clear elt */
            rb->B[R] = NULL;

            ++Ro;

            /* R = [0, 2L) */
            R = RINGBUF_NORMALIZE_OFFSET(Ro, L);

            uatomic_int_set(&rb->R, R);

            /* pop success */
            uatomic_int_zero(&rb->RLock);
            return 1;
        }

        uatomic_int_zero(&rb->RLock);
    }

    /* pop failed */
    return 0;
}


#define ringbuf_pop_always(rb, elt)    \
    while (ringbuf_pop((rb), &(elt)) != 1)

#define ringbuf_push_always(rb, elt)    \
    while (ringbuf_push((rb), (elt)) != 1)

#ifdef __cplusplus
}
#endif

#endif /* _RINGBUF_H_ */
