/***********************************************************************
* ©2013-2016 Cameron Desrochers.
* Distributed under the simplified BSD license (see the license file that
* should have come with this header).
* Uses Jeff Preshing's semaphore implementation (under the terms of its
* separate zlib license, embedded below).
*
* Provides portable (VC++2010+, Intel ICC 13, GCC 4.7+, and anything
*  C++11 compliant) implementation of low-level memory barriers, plus
*  a few semi-portable utility macros (for inlining and alignment).
* Also has a basic atomic type (limited to hardware-supported atomics
*  with no memory ordering guarantees).
* Uses the AE_* prefix for macros (historical reasons), and the
*  "moodycamel" namespace for symbols.
***********************************************************************/
/*
** @file      memord.h
**  Memory Order operations both for Windows and Linux.
**    https://git.project-hobbit.eu/dj16/ricec/raw/c9d3dceb1c3b1c03a42077e0461e3ce5a2615a51/data/atomicops.h
**    https://github.com/preshing/cpp11-on-multicore/blob/master/common/sema.h
**    https://www.cnblogs.com/lizhanzhe/p/10893016.html
** @author Cameron Desrochers
** @version 1.0.1
** @since 2020-12-08 10:46:50
** @date      2024-11-04 00:10:08
*/
#ifndef _MEMORY_ORDER_H_
#define _MEMORY_ORDER_H_

#if defined(__cplusplus)
extern "C"
{
#endif

#include <assert.h>

/* Platform detection */
#if defined(__INTEL_COMPILER)
# define AE_ICC
#elif defined(_MSC_VER)
# define AE_VCPP
#elif defined(__GNUC__)
# define AE_GCC
#endif

#if defined(_M_IA64) || defined(__ia64__)
# define AE_ARCH_IA64
#elif defined(_WIN64) || defined(__amd64__) || defined(_M_X64) || defined(__x86_64__)
# define AE_ARCH_X64
#elif defined(_M_IX86) || defined(__i386__)
# define AE_ARCH_X86
#elif defined(_M_PPC) || defined(__powerpc__)
# define AE_ARCH_PPC
#else
# define AE_ARCH_UNKNOWN
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

/* AE_ALIGN */
#if defined(AE_VCPP) || defined(AE_ICC)
# define AE_ALIGN(x) __declspec(align(x))
#elif defined(AE_GCC)
# define AE_ALIGN(x) __attribute__((aligned(x)))
#else
/* Assume GCC compliant syntax... */
# define AE_ALIGN(x) __attribute__((aligned(x)))
#endif


/* Portable atomic fences implemented below */

typedef enum {
    memory_order_relaxed,
    memory_order_acquire,
    memory_order_release,
    memory_order_acq_rel,
    memory_order_seq_cst,

    /**
      * memory_order_sync
      *  Forces a full sync:
      *  #LoadLoad, #LoadStore, #StoreStore, and most significantly, #StoreLoad
      */
    memory_order_sync = memory_order_seq_cst
} memory_order;


#if defined(AE_VCPP) || defined(AE_ICC)
// VS2010 and ICC13 don't support std::atomic_*_fence, implement our own fences

# include <intrin.h>

#if defined(AE_ARCH_X64) || defined(AE_ARCH_X86)
# define AeFullSync _mm_mfence
# define AeLiteSync _mm_mfence
#elif defined(AE_ARCH_IA64)
# define AeFullSync __mf
# define AeLiteSync __mf
#elif defined(AE_ARCH_PPC)
# include <ppcintrinsics.h>
# define AeFullSync __sync
# define AeLiteSync __lwsync
#endif


#ifdef AE_VCPP
# pragma warning(push)
// Disable erroneous 'conversion from long to unsigned int, signed/unsigned mismatch' error when using `assert`
# pragma warning(disable: 4365)
# ifdef __cplusplus_cli
#  pragma managed(push, off)
# endif
#endif


STATIC_INLINE void __mo_compiler_fence(memory_order order)
{
    switch (order) {
        case memory_order_relaxed:
            break;
        case memory_order_acquire:
            _ReadBarrier();
            break;
        case memory_order_release:
            _WriteBarrier();
            break;
        case memory_order_acq_rel:
            _ReadWriteBarrier();
            break;
        case memory_order_seq_cst:
            _ReadWriteBarrier();
            break;
        default:
            assert(0);
    }
}

/**
 * x86/x64 have a strong memory model -- all loads and stores have acquire and
 *  release semantics automatically (so only need compiler barriers for those).
 */
#if defined(AE_ARCH_X86) || defined(AE_ARCH_X64)

STATIC_INLINE void __mo_fence(memory_order order)
{
    switch (order) {
    case memory_order_relaxed:
        break;
    case memory_order_acquire:
        _ReadBarrier();
        break;
    case memory_order_release:
        _WriteBarrier();
        break;
    case memory_order_acq_rel:
        _ReadWriteBarrier();
        break;
    case memory_order_seq_cst:
        _ReadWriteBarrier();
        AeFullSync();
        _ReadWriteBarrier();
        break;
    default:
        assert(0);
        break;
    }
}

#else

STATIC_INLINE void __mo_fence(memory_order order)
{
    // Non-specialized arch, use heavier memory barriers everywhere just in case :-(
    switch (order) {
    case memory_order_relaxed:
        break;
    case memory_order_acquire:
        _ReadBarrier();
        AeLiteSync();
        _ReadBarrier();
        break;
    case memory_order_release:
        _WriteBarrier();
        AeLiteSync();
        _WriteBarrier();
        break;
    case memory_order_acq_rel:
        _ReadWriteBarrier();
        AeLiteSync();
        _ReadWriteBarrier();
        break;
    case memory_order_seq_cst:
        _ReadWriteBarrier();
        AeFullSync();
        _ReadWriteBarrier();
        break;
    default:
        assert(0);
        break;
    }
}
#endif

#elif defined(__cplusplus)
// Use standard library of atomics for cpp
# include <atomic>

STATIC_INLINE void __mo_compiler_fence(memory_order order)
{
    switch (order) {
    case memory_order_relaxed: break;
    case memory_order_acquire: std::atomic_signal_fence(std::memory_order_acquire); break;
    case memory_order_release: std::atomic_signal_fence(std::memory_order_release); break;
    case memory_order_acq_rel: std::atomic_signal_fence(std::memory_order_acq_rel); break;
    case memory_order_seq_cst: std::atomic_signal_fence(std::memory_order_seq_cst); break;
    default: assert(0);
    }
}

STATIC_INLINE void __mo_fence(memory_order order)
{
    switch (order) {
        case memory_order_relaxed: break;
        case memory_order_acquire: std::atomic_thread_fence(std::memory_order_acquire); break;
        case memory_order_release: std::atomic_thread_fence(std::memory_order_release); break;
        case memory_order_acq_rel: std::atomic_thread_fence(std::memory_order_acq_rel); break;
        case memory_order_seq_cst: std::atomic_thread_fence(std::memory_order_seq_cst); break;
        default: assert(0);
    }
}
#endif

#if defined(AE_VCPP) && (_MSC_VER < 1700 || defined(__cplusplus_cli))
# pragma warning(pop)
# ifdef __cplusplus_cli
#  pragma managed(pop)
# endif
#endif

#ifdef __cplusplus
}
#endif

#endif /* _MEMORY_ORDER_H_ */
