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
** @file      bo.h
**  Byte Converter for 2, 4, 8 bytes numeric types.
**
** Big Endian: XDR (big endian) encoding of numeric types
**
**                   Register
**                  0x0A0B0C0D
**      Memory         | | | |
**       |..|          | | | |
**  a+0: |0A|<---------+ | | |
**  a+1: |0B|<-----------+ | |
**  a+2: |0C|<-------------+ |
**  a+3: |0D|<---------------+
**       |..|
**
** Little Endian: NDR (little endian) encoding of numeric types
**
**   Register
**  0x0A0B0C0D
**     | | | |              Memory
**     | | | |               |..|
**     | | | +--------> a+0: |0D|
**     | | +----------> a+1: |0C|
**     | +------------> a+2: |0B|
**     +--------------> a+3: |0A|
**                           |..|
**
** @author Liang Zhang <350137278@qq.com>
** @version 0.0.24
** @since 2013-06-19 12:09:10
** @date      2024-11-03 22:48:50
*/
#ifndef BYTE_ORDER_H_INCLUDED
#define BYTE_ORDER_H_INCLUDED

#if defined(__cplusplus)
extern "C"
{
#endif

#include <memory.h>


#ifdef _MSC_VER
    #define __INLINE static __forceinline
    #define __INLINE_ALL __INLINE
#else
    #define __INLINE static inline
    #if (defined(__APPLE__) && defined(__ppc__))
        /* static inline __attribute__ here breaks osx ppc gcc42 build */
        #define __INLINE_ALL static __attribute__((always_inline))
    #else
        #define __INLINE_ALL static inline __attribute__((always_inline))
    #endif
#endif

/**
 * uniform int types
 */
#if defined (_SVR4) || defined (SVR4) || defined (__OpenBSD__) || \
    defined (_sgi) || defined (__sun) || defined (sun) || \
    defined (__digital__) || defined (__HP_cc)
    # include <inttypes.h>
#elif defined (_MSC_VER) && _MSC_VER < 1600
  # ifndef TYPEDEF_HAS_STDINT
  #   define TYPEDEF_HAS_STDINT
    /* VS 2010 (_MSC_VER 1600) has stdint.h */
    typedef __int8 int8_t;
    typedef unsigned __int8 uint8_t;
    typedef __int16 int16_t;
    typedef unsigned __int16 uint16_t;
    typedef __int32 int32_t;
    typedef unsigned __int32 uint32_t;
    typedef __int64 int64_t;
    typedef unsigned __int64 uint64_t;
  # endif
#elif defined (_AIX)
    # include <sys/inttypes.h>
#else
    # include <inttypes.h>
#endif

#ifndef HAS_BYTE_T_DEFINED
    typedef unsigned char byte_t;
    #define HAS_BYTE_T_DEFINED
#endif


static union {
    char c[4];
    uint8_t f;
} __endianess = {{'l','0','0','b'}};

#define _host_little_endian (((char)__endianess.f) == 'l')
#define _host_big_endian (((char)__endianess.f) == 'b')


/**
 * BO: Bit Op
 *
 * Setting a bit
 *   Use the bitwise OR operator (|) to set a bit.
 */
#define BO_set_bit(number, x) \
    (number) |= 1 << (x)

/**
 * Clearing a bit
 *   Use the bitwise AND operator (&) to clear a bit.
 *   That will clear bit x.You must invert the bit string with the bitwise NOT operator (~), then AND it.
 */
#define BO_clear_bit(number, x) \
    (number) &= ~(1 << (x))

/**
 * Toggling a bit
 *   The XOR operator (^) can be used to toggle a bit.
 */
#define BO_toggle_bit(number, x) \
    (number) ^= 1 << (x)

/**
 * Checking a bit
 *   To check a bit, shift the number x to the right, then bitwise AND it.
 */
#define BO_check_bit(number, x) \
    (((number) >> (x)) & 1)

/**
 * Changing the nth bit to x
 *   To check a bit, shift the number x to the right, then bitwise AND it.
 *   Setting the nth bit to either 1 or 0 can be achieved with the following
 *   Bit n will be set if x is 1, and cleared if x is 0.
 */
#define BO_change_bit(number, n, x) \
    (number) ^= (-(x) ^ (number)) & (1 << (n))


__INLINE void BO_swap_even_bytes(void *value, size_t size)
{
    /* size must be one of: 2, 4, 8 */
    size_t i;
    uint8_t t;
    uint8_t *b = (uint8_t*) value;
    for (i = 0; i < size/2; ++i) {
        t = b[i];
        b[i] = b[size-i-1];
        b[size-i-1] = t;
    }
}


#if defined (_MSC_VER) && _MSC_VER >= 1600
#   include <stdlib.h>
#   define BO_swap_word(wordPtr)  do { \
        uint16_t *_ub2addr = ((uint16_t*)(void*)(wordPtr)); \
        *_ub2addr = _byteswap_ushort(*_ub2addr); \
    } while(0)

#   define BO_swap_dword(dwordPtr)  do { \
        uint32_t *_ub4addr = ((uint32_t*)(void*)(dwordPtr)); \
        *_ub4addr = _byteswap_ulong(*_ub4addr); \
    } while(0)

#   define BO_swap_qword(qwordPtr)  do { \
        uint64_t *_ub8addr = ((uint64_t*)(void*)(qwordPtr)); \
        *_ub8addr = _byteswap_uint64(*_ub8addr); \
    } while(0)

__INLINE void BO_swap_bytes(void *value, size_t size)
{
    /* size must be one of: 2, 4, 8 */
    if (size == 2) {
        BO_swap_word(value);
    } else if (size == 4) {
        BO_swap_dword(value);
    } else if (size == 8) {
        BO_swap_qword(value);
    } else {
        BO_swap_even_bytes(value, size);
    }
}
#elif __GNUC__ > 4
#   define BO_swap_word(wordPtr)  do { \
        uint16_t *_ub2addr = ((uint16_t*)(void*)(wordPtr)); \
        *_ub2addr = __builtin_bswap16(*_ub2addr); \
    } while(0)

#   define BO_swap_dword(dwordPtr)  do { \
        uint32_t *_ub4addr = ((uint32_t*)(void*)(dwordPtr)); \
        *_ub4addr = __builtin_bswap32(*_ub4addr); \
    } while(0)

#   define BO_swap_qword(qwordPtr)  do { \
        uint64_t *_ub8addr = ((uint64_t*)(void*)(qwordPtr)); \
        *_ub8addr = __builtin_bswap64(*_ub8addr); \
    } while(0)

#   define BO_swap_bytes          BO_swap_even_bytes
#else
#   define BO_swap_word(wordPtr)     BO_swap_even_bytes((wordPtr), sizeof(ub2))
#   define BO_swap_dword(dwordPtr)   BO_swap_even_bytes((dwordPtr), sizeof(ub4))
#   define BO_swap_qword(qwordPtr)   BO_swap_even_bytes((qwordPtr), sizeof(ub8))
#   define BO_swap_bytes          BO_swap_even_bytes
#endif


__INLINE void BO_bytes_betoh(char *bytes, int size)
{
    if (_host_little_endian) {
        BO_swap_bytes((void*) bytes, size);
    }
}


__INLINE void BO_bytes_letoh(char *bytes, int size)
{
    if (_host_big_endian) {
        BO_swap_bytes((void*) bytes, size);
    }
}


__INLINE void BO_bytes_htobe(void *bytes, int size)
{
    if (_host_little_endian) {
        BO_swap_bytes((void*) bytes, size);
    }
}


__INLINE void BO_bytes_htole(void *bytes, int size)
{
    if (_host_big_endian) {
        BO_swap_bytes((void*) bytes, size);
    }
}


/**
 * 2 bytes numeric converter: int16_t, uint16_t
 */
__INLINE int16_t BO_i16_htole (int16_t val)
{
    if (_host_big_endian) {
        BO_swap_bytes(&val, sizeof(val));
    }
    return val;
}

__INLINE int16_t BO_i16_htobe (int16_t val)
{
    if (_host_little_endian) {
        BO_swap_bytes(&val, sizeof(val));
    }
    return val;
}

__INLINE int16_t BO_i16_letoh (int16_t val)
{
    if (_host_big_endian) {
        BO_swap_bytes(&val, sizeof(val));
    }
    return val;
}

__INLINE int16_t BO_i16_betoh (int16_t val)
{
    if (_host_little_endian) {
        BO_swap_bytes(&val, sizeof(val));
    }
    return val;
}

/**
 * 4 bytes numeric converter:
 *  int32_t, uint32_t
 *  float
 */
__INLINE int32_t BO_i32_htole (int32_t val)
{
    if (_host_big_endian) {
        BO_swap_bytes(&val, sizeof(val));
    }
    return val;
}

__INLINE int32_t BO_i32_htobe (int32_t val)
{
    if (_host_little_endian) {
        BO_swap_bytes(&val, sizeof(val));
    }
    return val;
}

__INLINE int32_t BO_i32_letoh (int32_t val)
{
    if (_host_big_endian) {
        BO_swap_bytes(&val, sizeof(val));
    }
    return val;
}

__INLINE int32_t BO_i32_betoh (int32_t val)
{
    if (_host_little_endian) {
        BO_swap_bytes(&val, sizeof(val));
    }
    return val;
}

__INLINE float BO_f32_htole (float val)
{
    if (_host_big_endian) {
        BO_swap_bytes(&val, sizeof(val));
    }
    return val;
}

__INLINE float BO_f32_htobe (float val)
{
    if (_host_little_endian) {
        BO_swap_bytes(&val, sizeof(val));
    }
    return val;
}

__INLINE float BO_f32_letoh (float val)
{
    if (_host_big_endian) {
        BO_swap_bytes(&val, sizeof(val));
    }
    return val;
}

__INLINE float BO_f32_betoh (float val)
{
    if (_host_little_endian) {
        BO_swap_bytes(&val, sizeof(val));
    }
    return val;
}

/**
 * 8 bytes numeric converter
 */
__INLINE double BO_f64_htole (double val)
{
    if (_host_big_endian) {
        BO_swap_bytes(&val, sizeof(val));
    }
    return val;
}

__INLINE double BO_f64_htobe (double val)
{
    if (_host_little_endian) {
        BO_swap_bytes(&val, sizeof(val));
    }
    return val;
}

__INLINE double BO_f64_letoh (double val)
{
    if (_host_big_endian) {
        BO_swap_bytes(&val, sizeof(val));
    }
    return val;
}

__INLINE double BO_f64_betoh (double val)
{
    if (_host_little_endian) {
        BO_swap_bytes(&val, sizeof(val));
    }
    return val;
}

__INLINE int64_t BO_i64_htole (int64_t val)
{
    if (_host_big_endian) {
        BO_swap_bytes(&val, sizeof(val));
    }
    return val;
}

__INLINE int64_t BO_i64_htobe (int64_t val)
{
    if (_host_little_endian) {
        BO_swap_bytes(&val, sizeof(val));
    }
    return val;
}

__INLINE int64_t BO_i64_letoh (int64_t val)
{
    if (_host_big_endian) {
        BO_swap_bytes(&val, sizeof(val));
    }
    return val;
}

__INLINE int64_t BO_i64_betoh (int64_t val)
{
    if (_host_little_endian) {
        BO_swap_bytes(&val, sizeof(val));
    }
    return val;
}

/**
 * read bytes to host
 */
__INLINE int16_t BO_bytes_betoh_i16 (void *b2)
{
    int16_t val = *(int16_t*) b2;
    if (_host_little_endian) {
        val = BO_i16_betoh (val);
    }
    return val;
}

__INLINE int16_t BO_bytes_letoh_i16 (void *b2)
{
    int16_t val = *(int16_t*) b2;
    if (_host_big_endian) {
        val = BO_i16_betoh (val);
    }
    return val;
}

__INLINE int32_t BO_bytes_betoh_i32 (void *b4)
{
    int32_t val = *(int32_t*) b4;
    if (_host_little_endian) {
        val = BO_i32_betoh (val);
    }
    return val;
}

__INLINE int32_t BO_bytes_letoh_i32 (void *b4)
{
    int32_t val = *(int32_t*) b4;
    if (_host_big_endian) {
        val = BO_i32_betoh (val);
    }
    return val;
}

__INLINE float BO_bytes_betoh_f32 (void *b4)
{
    float val = *(float*) b4;
    if (_host_little_endian) {
        val = BO_f32_betoh (val);
    }
    return val;
}

__INLINE float BO_bytes_letoh_f32 (void *b4)
{
    float val = *(float*) b4;
    if (_host_big_endian) {
        val = BO_f32_betoh (val);
    }
    return val;
}

__INLINE int64_t BO_bytes_betoh_i64 (void *b8)
{
    int64_t val = *(int64_t*) b8;
    if (_host_little_endian) {
        val = BO_i64_betoh (val);
    }
    return val;
}

__INLINE int64_t BO_bytes_letoh_i64 (void *b8)
{
    int64_t val = *(int64_t*) b8;
    if (_host_big_endian) {
        val = BO_i64_betoh (val);
    }
    return val;
}

__INLINE double BO_bytes_betoh_f64 (void *b8)
{
    double val = *(double*) b8;
    if (_host_little_endian) {
        val = BO_f64_betoh (val);
    }
    return val;
}

__INLINE double BO_bytes_letoh_f64 (void *b8)
{
    double val = *(double*) b8;
    if (_host_big_endian) {
        val = BO_f64_betoh (val);
    }
    return val;
}

#ifdef BYTEORDER_TEST
#include <assert.h>
#include <string.h>

static void byteorder_test_int16 (int16_t val)
{
    int16_t a, b;
    char b2[2];

    a = b = val;

    BO_swap_bytes(&a, sizeof(a));
    BO_swap_bytes(&a, sizeof(a));
    assert(a == b);

    b = BO_i16_htole(a);
    b = BO_i16_letoh(b);
    assert(a == b);

    b = BO_i16_htobe(a);
    b = BO_i16_betoh(b);
    assert(a == b);

    /* write as big endian to bytes */
    b = BO_i16_htobe(a);
    memcpy(b2, &b, sizeof(b));

    /* read bytes */
    b = * (int16_t*) b2;
    b = BO_i16_betoh(b);
    assert(a == b);

    b = BO_bytes_betoh_i16 (b2);
    assert(a == b);
}

static void byteorder_test_int32 (int val)
{
    int a, b;
    char b4[4];

    a = b = val;

    BO_swap_bytes(&a, sizeof(a));
    BO_swap_bytes(&a, sizeof(a));
    assert(a == b);

    b = BO_i32_htole(a);
    b = BO_i32_letoh(b);
    assert(a == b);

    b = BO_i32_htobe(a);
    b = BO_i32_betoh(b);
    assert(a == b);

    /* read bytes */
    b = BO_i32_htobe(a);
    memcpy(b4, &b, sizeof(b));
    b = BO_bytes_betoh_i32 (b4);
    assert(a == b);
}

static void byteorder_test_f32 (float val)
{
    float a, b;
    char b4[4];

    a = b = val;

    BO_swap_bytes(&a, sizeof(a));
    BO_swap_bytes(&a, sizeof(a));
    assert(a == b);

    b = BO_f32_htole(a);
    b = BO_f32_letoh(b);
    assert(a == b);

    b = BO_f32_htobe(a);
    b = BO_f32_betoh(b);
    assert(a == b);

    b = BO_f32_htobe(a);
    memcpy(b4, &b, sizeof(b));
    b = BO_bytes_betoh_f32 (b4);
    assert(a == b);
}

static void byteorder_test_f64 (double val)
{
    double a, b;
    char b8[8];

    a = b = val;

    BO_swap_bytes(&a, sizeof(a));
    BO_swap_bytes(&a, sizeof(a));
    assert(a == b);

    b = BO_f64_htole(a);
    b = BO_f64_letoh(b);
    assert(a == b);

    b = BO_f64_htobe(a);
    b = BO_f64_betoh(b);
    assert(a == b);

    b = BO_f64_htobe(a);
    memcpy(b8, &b, sizeof(b));
    b = BO_bytes_betoh_f64 (b8);
    assert(a == b);
}

#endif

#if defined(__cplusplus)
}
#endif

#endif /* BYTE_ORDER_H_INCLUDED */
