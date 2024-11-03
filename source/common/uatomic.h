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
** @file uatomic.h
**  user CAS atomic api both for Windows and Linux
**
** Usage:
**   uatomic_int  i;
**   uatomic_int_set(&i, 0);
**   uatomic_int_add(&i);
**
** @author Liang Zhang <350137278@qq.com>
** @version 0.0.4
** @since 2019-12-15 12:46:50
** @date 2024-11-04 02:14:49
*/
#ifndef _U_ATOMIC_H__
#define _U_ATOMIC_H__

#if defined(__cplusplus)
extern "C"
{
#endif

#if defined(_WIN32)
  // Windows
  # include <Winsock2.h>
  # include <Windows.h>

  # include <process.h>
  # include <stdint.h>
  # include <time.h>
  # include <limits.h>

  # ifdef _LINUX_GNUC
    # undef _LINUX_GNUC
  # endif

#elif defined(__GNUC__)
  // Linux
  # if __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 1)
    # error GCC version must be greater or equal than 4.1.2
  # endif

  # include <sched.h>
  # include <pthread.h>

  # include <unistd.h>
  # include <stdint.h>
  # include <time.h>
  # include <signal.h>
  # include <errno.h>
  # include <limits.h>

  # include <sys/time.h>
  # include <sys/sysinfo.h>

  # ifndef _LINUX_GNUC
    # define _LINUX_GNUC
  # endif
#else
  // TODO: MACOSX
  # error Currently only Windows and Linux os are supported.
#endif


#if defined(_LINUX_GNUC)
typedef volatile int         uatomic_int;

#   define uatomic_int_add(a)           __sync_add_and_fetch(a, 1)
#   define uatomic_int_sub(a)           __sync_sub_and_fetch(a, 1)
#   define uatomic_int_set(a, newval)   __sync_lock_test_and_set(a, (newval))
#   define uatomic_int_get(a)           __sync_fetch_and_add(a, 0)
#   define uatomic_int_zero(a)          __sync_lock_release(a)
#   define uatomic_int_comp_exch(a, comp, exch)  __sync_val_compare_and_swap(a, (comp), (exch))

#   define uatomic_int_add_n(a)         __sync_add_and_fetch(a, (n))
#   define uatomic_int_sub_n(a)         __sync_sub_and_fetch(a, (n))

typedef volatile int64_t     uatomic_int64;

#   define uatomic_int64_add(a)         uatomic_int_add(a)
#   define uatomic_int64_sub(a)         uatomic_int_sub(a)
#   define uatomic_int64_set(a, newval) uatomic_int_set(a, (newval))
#   define uatomic_int64_get(a)         uatomic_int_get(a)
#   define uatomic_int64_zero(a)        uatomic_int_zero(a)
#   define uatomic_int64_comp_exch(a, comp, exch)  uatomic_int_comp_exch(a, (comp), (exch))

#   define uatomic_int64_add_n(a, n)    uatomic_int_add_n(a, (n))
#   define uatomic_int64_sub_n(a, n)    uatomic_int_sub_n(a, (n))

typedef volatile void *      uatomic_ptr;

#   define uatomic_ptr_set(a, newval)   uatomic_int_set(((void**)(a)), (newval))
#   define uatomic_ptr_get(a)           uatomic_int_get(((void**)(a)))
#   define uatomic_ptr_zero(a)          uatomic_int_zero(((void**)(a)))
#   define uatomic_ptr_comp_exch(a, comp, exch)  uatomic_int_comp_exch(((void**)(a)), (comp), (exch))

#elif defined(_WIN32)
typedef volatile LONG        uatomic_int;

#   define uatomic_int_add(a)           InterlockedIncrement(a)
#   define uatomic_int_sub(a)           InterlockedDecrement(a)
#   define uatomic_int_set(a, newval)   InterlockedExchange(a, (newval))
#   define uatomic_int_get(a)           InterlockedCompareExchange(a, 0, 0)
#   define uatomic_int_zero(a)          InterlockedExchange(a, 0)
#   define uatomic_int_comp_exch(a, comp, exch)    InterlockedCompareExchange(a, (exch), (comp))

#   define uatomic_int_add_n(a, n)      InterlockedAnd(a, (n))
#   define uatomic_int_sub_n(a, n)      InterlockedAnd(a, -(n))

typedef volatile LONG64      uatomic_int64;

#   define uatomic_int64_add(a)         InterlockedIncrement64(a)
#   define uatomic_int64_sub(a)         InterlockedDecrement64(a)
#   define uatomic_int64_set(a, newval) InterlockedExchange64(a, (newval))
#   define uatomic_int64_get(a)         InterlockedCompareExchange64(a, 0, 0)
#   define uatomic_int64_zero(a)        InterlockedExchange64(a, 0)
#   define uatomic_int64_comp_exch(a, comp, exch)    InterlockedCompareExchange64(a, (exch), (comp))

#   define uatomic_int64_add_n(a, n)    InterlockedAnd64(a, (n))
#   define uatomic_int64_sub_n(a, n)    InterlockedAnd64(a, -(n))

typedef volatile PVOID       uatomic_ptr;

#   define uatomic_ptr_set(a, newval)   InterlockedExchangePointer(a, (newval))
#   define uatomic_ptr_get(a)           InterlockedCompareExchangePointer(a, 0, 0)
#   define uatomic_ptr_zero(a)          InterlockedExchangePointer(a, 0)
#   define uatomic_ptr_comp_exch(a, comp, exch)  InterlockedCompareExchangePointer(a, (exch), (comp))

#else
#   error Currently only Windows and Linux os are supported.
#endif

#ifdef __cplusplus
}
#endif

#endif /* _U_ATOMIC_H__ */
