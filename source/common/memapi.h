/******************************************************************************
* Copyright © 2024-2035 Light Zhang <mapaware@hotmail.com>, MapAware, Inc.
* ALL RIGHTS RESERVED.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
******************************************************************************/
/*
** @file memapi.h
** @brief memory helper api both for linux and windows.
**
** @author mapaware@hotmail.com
** @copyright © 2024-2030 mapaware.top All Rights Reserved.
** @version 0.0.29
**
** @since 2018-10-25 09:09:10
** @date 2024-11-03 23:02:40
**
** @note
*/
#ifndef MEMAPI_H_INCLUDED
#define MEMAPI_H_INCLUDED

#if defined(__cplusplus)
extern "C"
{
#endif

#include "mscrtdbg.h"

#include <assert.h>  /* assert */
#include <string.h>  /* memset */
#include <stdio.h>   /* printf, perror */
#include <limits.h>  /* realpath, PATH_MAX=4096 */
#include <stdbool.h> /* memset */
#include <ctype.h>
#include <stdlib.h>  /* malloc, alloc */
#include <malloc.h>  /* alloca */
#include <errno.h>


#if defined (_MSC_VER)
    # define MEMAPI_ENABLE_ALLOCA    0

    /* Microsoft Windows */
    # if !defined(__MINGW64__) && !defined(__MINGW32__) && !defined(__MINGW__)
        # pragma warning(push)
        # pragma warning(disable : 4996)
    # endif
#else
    # define MEMAPI_ENABLE_ALLOCA    1

    # ifdef MEMAPI_USE_LIBJEMALLOC
        /* need to link: libjemalloc.a */
        # include <jemalloc/jemalloc.h>
    # endif
#endif


#ifndef NOWARNING_UNUSED
    # if defined(__GNUC__) || defined(__CYGWIN__)
        # define NOWARNING_UNUSED(x) __attribute__((unused)) x
    # else
        # define NOWARNING_UNUSED(x) x
    # endif
#endif


#ifndef STATIC_INLINE
    # if defined(_MSC_VER)
        # define STATIC_INLINE  NOWARNING_UNUSED(static) __forceinline
    # elif defined(__GNUC__) || defined(__CYGWIN__)
        # define STATIC_INLINE  NOWARNING_UNUSED(static) __attribute__((always_inline)) inline
    # else
        # define STATIC_INLINE  NOWARNING_UNUSED(static)
    # endif
#endif


#define memapi_align_bsize(bsz, alignsize)  \
        ((size_t)((((size_t)(bsz)+(alignsize)-1)/(alignsize))*(alignsize)))

#define memapi_align_psize(psz)  memapi_align_bsize(psz, sizeof(void *))


#define memapi_oom_check(p) do { \
        if (!(p)) { \
            printf("FATAL: (memapi.h:%d) out of memory.\n", __LINE__); \
            exit(EXIT_FAILURE); \
        } \
    } while(0)


/**
 * mem_alloc_zero() allocates memory for an array of nmemb elements of
 *  size bytes each and returns a pointer to the allocated memory.
 * THE MEMORY IS SET TO ZERO.
 */
STATIC_INLINE void * mem_alloc_zero (int nmemb, size_t size)
{
    void * p =
        #ifdef MEMAPI_USE_LIBJEMALLOC
            je_calloc(nmemb, size);
        #else
            calloc(nmemb, size);
        #endif

    memapi_oom_check(p);
    return p;
}


/**
 * mem_alloc_unset() allocate with THE MEMORY NOT BE INITIALIZED.
 */
STATIC_INLINE void * mem_alloc_unset (size_t size)
{
    void * p =
        #ifdef MEMAPI_USE_LIBJEMALLOC
            je_malloc(size);
        #else
            malloc(size);
        #endif
    memapi_oom_check(p);
    return p;
}


/**
 * mem_realloc() changes the size of the memory block pointed to by ptr
 *  to size bytes. The contents will be unchanged in the range from the
 *  start of the region up to the minimum of the old and new sizes.
 * If the new size is larger than the old size,
 *  THE ADDED MEMORY WILL NOT BE INITIALIZED.
 */
STATIC_INLINE void * mem_realloc (void * ptr, size_t size)
{
    void *np =
        #ifdef MEMAPI_USE_LIBJEMALLOC
            je_realloc(ptr, size);
        #else
            realloc(ptr, size);
        #endif
    memapi_oom_check(np);
    return np;
}


STATIC_INLINE char * mem_strdup (const char *s)
{
    if (s) {
        size_t sz = strlen(s) + sizeof(char);
        char * dst = (char *) mem_alloc_unset(sz);
        memcpy(dst, s, sz);
        return dst;
    }
    return 0;
}


STATIC_INLINE char * mem_strdup_len (const char *s, int len)
{
    char *dst = 0;
    if (len < 0) {
        len = (int)(s? strlen(s) : 0);
    }
    dst = (char *) mem_alloc_unset(len + sizeof(char));
    if (len > 0) {
        memcpy(dst, s, len);
    }
    dst[len] = '\0';
    return dst;
}


/**
 * mem_free() frees the memory space pointed to by ptr, which must have
 *  been returned by a previous call to malloc(), calloc() or realloc().
 *  IF PTR IS NULL, NO OPERATION IS PERFORMED.
 */
STATIC_INLINE void mem_free (void * ptr)
{
    if (ptr) {
    #ifdef MEMAPI_USE_LIBJEMALLOC
        je_free(ptr);
    #else
        free(ptr);
    #endif
    }
}


/**
 * mem_free_s() frees the memory pointed by the address of ptr and set
 *  ptr to zero. it is a safe version if mem_free().
 */
STATIC_INLINE void mem_free_s (void **pptr)
{
    if (pptr) {
        void *ptr = *pptr;

        if (ptr) {
            *pptr = 0;

        #ifdef MEMAPI_USE_LIBJEMALLOC
            je_free(ptr);
        #else
            free(ptr);
        #endif
        }
    }
}


typedef struct {
    void (*freebufcb)(void *);
    size_t bufsize;
    char buffer[0];
} alloca_buf_t;


STATIC_INLINE char * alloca_buf_new (size_t smallbufsize)
{
    size_t bufsize = memapi_align_psize(smallbufsize);
    alloca_buf_t *albuf =
#if MEMAPI_ENABLE_ALLOCA == 1
        (alloca_buf_t *)alloca(sizeof(alloca_buf_t) + sizeof(char) * bufsize);
#else
        NULL;
#endif

    if (albuf) {
        albuf->freebufcb = NULL;
    } else {
        albuf = (alloca_buf_t *) mem_alloc_unset(sizeof(alloca_buf_t) + sizeof(char) * bufsize);
        albuf->freebufcb = mem_free;
    }
    albuf->bufsize = bufsize;
    return (char *)albuf->buffer;
}


STATIC_INLINE void alloca_buf_free (char *buffer)
{
    alloca_buf_t *albuf = (alloca_buf_t *)(buffer - sizeof(alloca_buf_t));
    if (albuf->freebufcb) {
        albuf->freebufcb(albuf);
    }
}

#if defined (_MSC_VER)
    # pragma warning(pop)
#endif

#ifdef __cplusplus
}
#endif

#endif /* MEMAPI_H_INCLUDED */
