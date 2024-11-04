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
***********************************************************************/
/*
** @file      unsema.h
**  Unnamed Semaphore API for both Windows and Linux
**
** @author  Liang Zhang <350137278@qq.com>
** @version 0.0.3
** @since 2019-12-12 11:32:50
** @date      2024-11-04 01:46:51
*/
#ifndef _UN_SEMA_H__
#define _UN_SEMA_H__

#if defined(__cplusplus)
extern "C"
{
#endif

#ifndef _MSC_VER
  # include <unistd.h>
#endif

#include <time.h>

#include "basetype.h"

#ifdef __WINDOWS__
    typedef  HANDLE unsema_t;
#else
    # include <semaphore.h>
    typedef  sem_t  unsema_t;
#endif


#define UNSEMA_VALUE_MAX     SB2MAXVAL

#ifdef SEM_VALUE_MAX
# undef UNSEMA_VALUE_MAX
# define UNSEMA_VALUE_MAX    SEM_VALUE_MAX
#endif

#ifdef _POSIX_SEM_VALUE_MAX
# undef UNSEMA_VALUE_MAX
# define UNSEMA_VALUE_MAX    _POSIX_SEM_VALUE_MAX
#endif

#if UNSEMA_VALUE_MAX > SB2MAXVAL
# undef UNSEMA_VALUE_MAX
# define UNSEMA_VALUE_MAX    SB2MAXVAL
#endif


NOWARNING_UNUSED(static) int unsema_init (unsema_t *sema, int initval)
{
    int err = -1;

#ifdef __WINDOWS__
    unsema_t h = CreateSemaphoreA(NULL, initval, SB4MAXVAL, NULL);
    if (h) {
        *sema = h;
        err = 0;
    }
#else
    err = sem_init(sema, 0, initval);
#endif

    return err;
}


NOWARNING_UNUSED(static) int unsema_uninit (unsema_t *sema)
{
    int err = -1;

#ifdef __WINDOWS__
    if (CloseHandle(*sema)) {
        *sema = NULL;
        err = 0;
    }
#else
    err = sem_destroy(sema);
#endif

    return err;
}


NOWARNING_UNUSED(static) int unsema_wait (unsema_t *sema)
{
    int err = -1;

#ifdef __WINDOWS__
    if (WAIT_OBJECT_0 == WaitForSingleObject(*sema, INFINITE)) {
        // success
        err = 0;
    } else {
        err = 1;
    }
#else
    err = sem_wait(sema);
#endif

    return err;
}


NOWARNING_UNUSED(static) int unsema_timedwait (unsema_t *sema, ub4 milliseconds)
{
    int err = -1;

#ifdef __WINDOWS__
    if (WAIT_OBJECT_0 == WaitForSingleObject(*sema, (DWORD) milliseconds)) {
        // success
        err = 0;
    } else {
        // other errors
        err = 1;
    }
#else // Linux
    struct timespec ts;

    #ifdef UNSEMA_USES_GETTIMEOFDAY
        struct timeval tv;
        gettimeofday(&tv, 0);
        ts.tv_sec = tv.tv_sec + milliseconds/1000;
        ts.tv_nsec = tv.tv_usec * 1000 + (milliseconds % 1000) * 1000000;
    #else
        clock_gettime(CLOCK_REALTIME, &ts);
        ts.tv_sec += (milliseconds/1000);
        ts.tv_nsec += ((milliseconds % 1000) * 1000000);
    #endif

    err = sem_timedwait(sema, &ts);
#endif

    return err;
}


NOWARNING_UNUSED(static) int unsema_post (unsema_t *sema)
{
    int err = -1;

#ifdef __WINDOWS__
    if (ReleaseSemaphore(*sema, 1, NULL)) {
        // success
        err = 0;
    }
#else
    err = sem_post(sema);
#endif

    return err;
}


/**
 * get previous value and post
 */
NOWARNING_UNUSED(static) int unsema_postget (unsema_t *sema)
{
#ifdef __WINDOWS__
    LONG lCount;
    if (ReleaseSemaphore(*sema, 1, &lCount)) {
        // success
        return (int) lCount;
    }
#else
    int value;
    if (sem_getvalue(sema, &value) == 0 && sem_post(sema) == 0) {
        // success
        return value;
    }
#endif
    // error
    return (-1);
}


#ifdef __cplusplus
}
#endif

#endif /* _UN_SEMA_H__ */
