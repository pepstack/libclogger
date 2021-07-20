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
 * @filename   refcobject.h
 *  C reference count object api.
 *
 * @author     Liang Zhang <350137278@qq.com>
 * @version    0.0.2
 * @create     2020-12-09 21:12:10
 * @update     2021-07-20 12:23:00
 */
#ifndef _REFC_OBJECT_H_
#define _REFC_OBJECT_H_

#if defined(__cplusplus)
extern "C"
{
#endif

#include "uatomic.h"
#include "thread_rwlock.h"

#define REFCOBJ_ALIGN_SIZE(sz)   ((((size_t)(sz) + sizeof(void*) - sizeof(char))/sizeof(void*)) * sizeof(void*))

#define REFCOBJ_CAST_OBJ(pv)  refc_object obj = (refc_object) pv; --obj


typedef struct
{
    void (*__final)(void *);
    ThreadRWLock_t __rwlock;
    uatomic_int  __refc;
    int type;
    void *addr[0];
} refc_object_t, *refc_object;


STATIC_INLINE void * refc_object_new (int type, size_t elemsz, void (*finalize)(void *))
{
    refc_object p = (refc_object) mem_alloc_zero(1, REFCOBJ_ALIGN_SIZE(sizeof(*p) + elemsz));
    p->__refc = 1;
    p->__final = finalize;
    p->type = type;
    RWLockInit(&p->__rwlock);
    return p->addr;
}


STATIC_INLINE void* refc_object_inc (void **ppv)
{
    refc_object p = (refc_object)(*ppv);
    if (p--) {
        if (uatomic_int_add(&p->__refc) > 0) {
            return (p->addr);
        }
    }
    return NULL;
}


STATIC_INLINE void refc_object_dec (void **ppv)
{
    refc_object p = (refc_object)(*ppv);
    if (p--) {
        if (uatomic_int_sub(&p->__refc) <= 0) {
            *ppv = NULL;
            p->__final(p->addr);
            RWLockUninit(&p->__rwlock);
            mem_free(p);
        }
    }
}


STATIC_INLINE int refc_object_type (void *pv)
{
    REFCOBJ_CAST_OBJ(pv);
    return obj->type;
}


/* lock for read only */
STATIC_INLINE int refc_object_lock_rd (void *pv, int istry)
{
    REFCOBJ_CAST_OBJ(pv);
    return RWLockAcquire(&obj->__rwlock, RWLOCK_STATE_READ, istry);
}


/* unlock for read only */
STATIC_INLINE int refc_object_unlock_rd (void *pv)
{
    REFCOBJ_CAST_OBJ(pv);
    return RWLockRelease(&obj->__rwlock, RWLOCK_STATE_READ);
}


/* lock for write and read */
STATIC_INLINE int refc_object_lock (void *pv, int istry)
{
    REFCOBJ_CAST_OBJ(pv);
    return RWLockAcquire(&obj->__rwlock, RWLOCK_STATE_WRITE, istry);
}


/* unlock for write and read */
STATIC_INLINE int refc_object_unlock (void *pv)
{
    REFCOBJ_CAST_OBJ(pv);
    return RWLockRelease(&obj->__rwlock, RWLOCK_STATE_WRITE);
}

#ifdef __cplusplus
}
#endif

#endif /* _REFC_OBJECT_H_ */
