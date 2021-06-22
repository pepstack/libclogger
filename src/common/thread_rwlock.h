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
 * @filename   thread_rwlock.h
 *  thread safe lock for both Windows and Linux.
 *
 * @author     Liang Zhang <350137278@qq.com>
 * @version    0.0.3
 * @create     2019-11-14 16:35:07
 * @update     2019-11-22 12:36:34
 */
#ifndef THREAD_RWLOCK_H_INCLUDED
#define THREAD_RWLOCK_H_INCLUDED

#if defined(__cplusplus)
extern "C"
{
#endif

//-------------------------------------
#if defined(_WIN32)

# include <Windows.h>
  /* SRWLock functions */
  typedef void(CALLBACK *LPFN_SRWLOCK_API)(PSRWLOCK);
  typedef BOOLEAN(CALLBACK *LPFN_SRWLOCK_TRYAPI)(PSRWLOCK);
  typedef BOOL(CALLBACK *LPFN_SRWLOCK_SLEEPAPI)(PCONDITION_VARIABLE, PSRWLOCK, DWORD, ULONG);

  /* Critical section functions */
  typedef void(CALLBACK *LPFN_CRITSEC_API)(LPCRITICAL_SECTION);
  typedef BOOL(CALLBACK *LPFN_CRITSEC_TRYAPI)(LPCRITICAL_SECTION);

#else

  /* https://linux.die.net/man/3/pthread_rwlock_wrlock */
# ifndef __USE_GNU
#   define __USE_GNU
# endif

# include <sys/types.h>
# include <sys/sysinfo.h>
# include <unistd.h>
# include <sched.h>
# include <ctype.h>
# include <pthread.h>

#endif
//-------------------------------------


#ifndef NOWARNING_UNUSED
# if defined(__GNUC__) || defined(__CYGWIN__)
#   define NOWARNING_UNUSED(x) __attribute__((unused)) x
# else
#   define NOWARNING_UNUSED(x) x
# endif
#endif


#ifndef STATIC_INLINE
# if defined(_MSC_VER)
#   define STATIC_INLINE  NOWARNING_UNUSED(static) __forceinline
# elif defined(__GNUC__) || defined(__CYGWIN__)
#   define STATIC_INLINE  NOWARNING_UNUSED(static) __attribute__((always_inline)) inline
# else
#   define STATIC_INLINE  NOWARNING_UNUSED(static)
# endif
#endif


typedef enum _RWLockStateEnum {
    RWLOCK_STATE_READ  = 0,
    RWLOCK_STATE_WRITE = 1
} RWLockStateEnum;


typedef struct _ThreadRWLock_t
{
#if defined(_WIN32)
    /**
     * SRWLock functions - vs2005 and after
     *
     * synchapi.h(include Windows 7, Windows Server 2008 Windows Server 2008 R2, Windows.h)
     *   https://docs.microsoft.com/en-us/windows/win32/sync/slim-reader-writer--srw--locks
     *   https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-initializesrwlock
     *
     * Exclusive for Write, Shared for Read
     */
    SRWLOCK srwLock;

    LPFN_SRWLOCK_API      lpfnInitializeSRWLock;
    LPFN_SRWLOCK_API      lpfnAcquireSRWLockExclusive;
    LPFN_SRWLOCK_API      lpfnAcquireSRWLockShared;
    LPFN_SRWLOCK_API      lpfnReleaseSRWLockExclusive;
    LPFN_SRWLOCK_API      lpfnReleaseSRWLockShared;
    LPFN_SRWLOCK_TRYAPI   lpfnTryAcquireSRWLockExclusive;
    LPFN_SRWLOCK_TRYAPI   lpfnTryAcquireSRWLockShared;
    LPFN_SRWLOCK_SLEEPAPI lpfnSleepConditionVariableSRW;

    /* Critical section functions */
    CRITICAL_SECTION critSec;

    LPFN_CRITSEC_API      lpfnInitializeCriticalSection;
    LPFN_CRITSEC_API      lpfnDeleteCriticalSection;
    LPFN_CRITSEC_API      lpfnEnterCriticalSection;
    LPFN_CRITSEC_API      lpfnLeaveCriticalSection;
    LPFN_CRITSEC_TRYAPI   lpfnTryEnterCriticalSection;
#else
    /* Linux */
    pthread_rwlock_t rwlock;
#endif

} ThreadRWLock_t;


NOWARNING_UNUSED(static) void RWLockInit (ThreadRWLock_t *lock)
{
    memset(lock, 0, sizeof(*lock));

#if defined(_WIN32)
    HMODULE hModule = GetModuleHandle("kernel32.dll");
    if (hModule) {
        lock->lpfnInitializeSRWLock  = (LPFN_SRWLOCK_API)      GetProcAddress(hModule, "InitializeSRWLock");
        if (lock->lpfnInitializeSRWLock) {
            lock->lpfnAcquireSRWLockExclusive        = (LPFN_SRWLOCK_API)      GetProcAddress(hModule, "AcquireSRWLockExclusive");
            lock->lpfnAcquireSRWLockShared           = (LPFN_SRWLOCK_API)      GetProcAddress(hModule, "AcquireSRWLockShared");
            lock->lpfnReleaseSRWLockExclusive        = (LPFN_SRWLOCK_API)      GetProcAddress(hModule, "ReleaseSRWLockExclusive");
            lock->lpfnReleaseSRWLockShared           = (LPFN_SRWLOCK_API)      GetProcAddress(hModule, "ReleaseSRWLockShared");
            lock->lpfnTryAcquireSRWLockExclusive     = (LPFN_SRWLOCK_TRYAPI)   GetProcAddress(hModule, "TryAcquireSRWLockExclusive");
            lock->lpfnTryAcquireSRWLockShared        = (LPFN_SRWLOCK_TRYAPI)   GetProcAddress(hModule, "TryAcquireSRWLockShared");
            lock->lpfnSleepConditionVariableSRW      = (LPFN_SRWLOCK_SLEEPAPI) GetProcAddress(hModule, "SleepConditionVariableSRW");
        }
        else {
            lock->lpfnInitializeCriticalSection      = (LPFN_CRITSEC_API)      GetProcAddress(hModule, "InitializeCriticalSection");
            lock->lpfnDeleteCriticalSection          = (LPFN_CRITSEC_API)      GetProcAddress(hModule, "DeleteCriticalSection");
            lock->lpfnEnterCriticalSection           = (LPFN_CRITSEC_API)      GetProcAddress(hModule, "EnterCriticalSection");
            lock->lpfnLeaveCriticalSection           = (LPFN_CRITSEC_API)      GetProcAddress(hModule, "LeaveCriticalSection");
            lock->lpfnTryEnterCriticalSection        = (LPFN_CRITSEC_TRYAPI)   GetProcAddress(hModule, "TryEnterCriticalSection");
        }
    }

    if (lock->lpfnInitializeSRWLock) {
        lock->lpfnInitializeSRWLock(&lock->srwLock);
    }
    else {
        lock->lpfnInitializeCriticalSection(&lock->critSec);
    }
#else
    // Linux
    if (pthread_rwlock_init(&lock->rwlock, NULL) != 0) {
        exit(EXIT_FAILURE);
    }
#endif
}


NOWARNING_UNUSED(static) void RWLockUninit(ThreadRWLock_t *lock)
{
#if defined(_WIN32)
    if (lock->lpfnDeleteCriticalSection) {
        lock->lpfnDeleteCriticalSection(&lock->critSec);
    }
#else
    // Linux
    pthread_rwlock_destroy(&lock->rwlock);
#endif
}


STATIC_INLINE int RWLockAcquire(ThreadRWLock_t *lock, RWLockStateEnum lockState, int tryLock)
{
    // success
    int err = 0;

#if defined(_WIN32)
    if (lockState == RWLOCK_STATE_WRITE) {
        if (tryLock) {
            if (lock->lpfnTryAcquireSRWLockExclusive) {
                if (!lock->lpfnTryAcquireSRWLockExclusive(&lock->srwLock)) {
                    err = 1;
                }
            }
            else {
                if (!lock->lpfnTryEnterCriticalSection(&lock->critSec)) {
                    err = 1;
                }
            }
        }
        else {
            if (lock->lpfnAcquireSRWLockExclusive) {
                lock->lpfnAcquireSRWLockExclusive(&lock->srwLock);
            }
            else {
                lock->lpfnEnterCriticalSection(&lock->critSec);
            }
        }
    }
    else {
        if (tryLock) {
            if (lock->lpfnTryAcquireSRWLockShared) {
                if (!lock->lpfnTryAcquireSRWLockShared(&lock->srwLock)) {
                    err = 1;
                }
            }
            else {
                if (!lock->lpfnTryEnterCriticalSection(&lock->critSec)) {
                    err = 1;
                }
            }
        }
        else {
            if (lock->lpfnAcquireSRWLockShared) {
                lock->lpfnAcquireSRWLockShared(&lock->srwLock);
            }
            else {
                lock->lpfnEnterCriticalSection(&lock->critSec);
            }
        }
    }
#else
    // Linux
    if (lockState == RWLOCK_STATE_WRITE) {
        if (tryLock) {
            err = pthread_rwlock_trywrlock(&lock->rwlock);
        }
        else {
            err = pthread_rwlock_wrlock(&lock->rwlock);
        }
    }
    else {
        if (tryLock) {
            err = pthread_rwlock_tryrdlock(&lock->rwlock);
        }
        else {
            err = pthread_rwlock_rdlock(&lock->rwlock);
        }
    }
#endif
    // Linux: If successful, shall return zero;
    //  otherwise, an error number shall be returned to indicate the error.
    //
    // Windows:
    //   if tryBool is 0, always success with returned zero;
    //   if tryBool not 0, returns non-zero(1) indicates error.
    return err;
}


STATIC_INLINE int RWLockRelease(ThreadRWLock_t *lock, RWLockStateEnum lockState)
{
    int err = 0;

#if defined(_WIN32)
    if (lockState == RWLOCK_STATE_WRITE) {
        if (lock->lpfnReleaseSRWLockExclusive) {
            lock->lpfnReleaseSRWLockExclusive(&lock->srwLock);
        }
        else {
            lock->lpfnLeaveCriticalSection(&lock->critSec);
        }
    }
    else {
        if (lock->lpfnReleaseSRWLockShared) {
            lock->lpfnReleaseSRWLockShared(&lock->srwLock);
        }
        else {
            lock->lpfnLeaveCriticalSection(&lock->critSec);
        }
    }
#else
    // Linux
    err = pthread_rwlock_unlock(&lock->rwlock);
#endif

    // Linux: If successful, shall return zero;
    //  otherwise, an error number shall be returned to indicate the error.
    //
    // Windows: always success with returned zero;
    return err;
}


#if defined(__cplusplus)
}
#endif

#endif /* THREAD_RWLOCK_H_INCLUDED */
