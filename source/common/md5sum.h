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
** @file      md5sum.h
**  MD5 implementation.
**
**   md5sum("hello") == echo -n "hello" | md5sum
**
**   md5("hello") = {5d41402abc4b2a76b9719d911017c592}
**
** Usage:
**
**   char hash[MD5_SUM_LEN + 1];
**   char msg[] = "350137278@qq.com";
**
**   md5sum_t ctx;
**   md5sum_init(&ctx, 0);
**   md5sum_updt(&ctx, msg, strlen(msg));
**   md5sum_done(&ctx, ctx.digest);
**
**   md5_format_lower(ctx.digest, hash);
**
**   printf(">>>> {%s}\n", hash);
**
**   >>>> {bdc0bb1f6bea9f3b546657614918bc1d}
**   char bbuf[MD5_CHUNK_SIZE];
**   md5file("/root/Downloads/ebooks1.tar.gz", 0, ctx.digest, bbuf, sizeof(bbuf));
**
**   md5_format_upper(ctx.digest, hash);
**   printf("%s\n", hash);
**
** @author Liang Zhang <350137278@qq.com>
** @version 1.0.2
** @since 2017-08-28 10:21:09
** @date      2024-11-04 12:38:27
*/
#ifndef MD5_SUM_H__
#define MD5_SUM_H__


#if defined(__cplusplus)
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>


/**
 * fix length string buffer with 32 chars
 */
#define MD5_SUM_LEN       32

/**
 * size in byets for read file
 * MUST = 64 x N (N = 8, 16, ...)
 */
#ifndef MD5_CHUNK_SIZE
#  define MD5_CHUNK_SIZE  4096
#endif

typedef struct {
    uint32_t count[2];
    uint32_t state[4];
    uint8_t buffer[64];
    uint8_t digest[16];
} md5sum_t;


/**
 * private typedef and functions
 */
static const uint8_t __md5sum_padding__[] =
{
    0x80, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0,
       0, 0, 0, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0
};


#define __md5sum_F__(x,y,z) ((x & y) | (~x & z))

#define __md5sum_G__(x,y,z) ((x & z) | (y & ~z))

#define __md5sum_H__(x,y,z) (x^y^z)

#define __md5sum_I__(x,y,z) (y ^ (x | ~z))

#define __md5sum_LR__(x,n) ((x << n) | (x >> (32-n)))

#define __md5sum_FF__(a,b,c,d,x,s,ac) do { \
        a += __md5sum_F__(b,c,d) + x + ac; \
        a = __md5sum_LR__(a,s); \
        a += b; \
    } while(0)

#define __md5sum_GG__(a,b,c,d,x,s,ac) do { \
        a += __md5sum_G__(b,c,d) + x + ac; \
        a = __md5sum_LR__(a,s); \
        a += b; \
    } while(0)

#define __md5sum_HH__(a,b,c,d,x,s,ac) do { \
        a += __md5sum_H__(b,c,d) + x + ac; \
        a = __md5sum_LR__(a,s); \
        a += b; \
    } while(0)

#define __md5sum_II__(a,b,c,d,x,s,ac) do { \
        a += __md5sum_I__(b,c,d) + x + ac; \
        a = __md5sum_LR__(a,s); \
        a += b; \
    } while(0)


#define __md5sum_encode__(output, input, len) do { \
        uint32_t i = 0; \
        uint32_t j = 0; \
        while (j < len) { \
            output[j] = input[i] & 0xFF; \
            output[j+1] = (input[i] >> 8) & 0xFF; \
            output[j+2] = (input[i] >> 16) & 0xFF; \
            output[j+3] = (input[i] >> 24) & 0xFF; \
            i++; \
            j += 4; \
        } \
    } while(0)


#define __md5sum_decode__(output, input, len) do { \
        uint32_t i = 0; \
        uint32_t j = 0; \
        while (j < len) { \
            output[i] = (input[j]) | (input[j+1] << 8) | (input[j+2] << 16) | (input[j+3] << 24); \
            i++; \
            j += 4; \
        } \
    } while(0)


static void __md5sum_trans__(uint32_t state[4], const uint8_t block[64])
{
    uint32_t x[64];

    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];

    __md5sum_decode__(x, block, 64);

    /* Round 1 */
    __md5sum_FF__(a, b, c, d, x[ 0], 7, 0xd76aa478);
    __md5sum_FF__(d, a, b, c, x[ 1], 12, 0xe8c7b756);
    __md5sum_FF__(c, d, a, b, x[ 2], 17, 0x242070db);
    __md5sum_FF__(b, c, d, a, x[ 3], 22, 0xc1bdceee);
    __md5sum_FF__(a, b, c, d, x[ 4], 7, 0xf57c0faf);
    __md5sum_FF__(d, a, b, c, x[ 5], 12, 0x4787c62a);
    __md5sum_FF__(c, d, a, b, x[ 6], 17, 0xa8304613);
    __md5sum_FF__(b, c, d, a, x[ 7], 22, 0xfd469501);
    __md5sum_FF__(a, b, c, d, x[ 8], 7, 0x698098d8);
    __md5sum_FF__(d, a, b, c, x[ 9], 12, 0x8b44f7af);
    __md5sum_FF__(c, d, a, b, x[10], 17, 0xffff5bb1);
    __md5sum_FF__(b, c, d, a, x[11], 22, 0x895cd7be);
    __md5sum_FF__(a, b, c, d, x[12], 7, 0x6b901122);
    __md5sum_FF__(d, a, b, c, x[13], 12, 0xfd987193);
    __md5sum_FF__(c, d, a, b, x[14], 17, 0xa679438e);
    __md5sum_FF__(b, c, d, a, x[15], 22, 0x49b40821);

    /* Round 2 */
    __md5sum_GG__(a, b, c, d, x[ 1], 5, 0xf61e2562);
    __md5sum_GG__(d, a, b, c, x[ 6], 9, 0xc040b340);
    __md5sum_GG__(c, d, a, b, x[11], 14, 0x265e5a51);
    __md5sum_GG__(b, c, d, a, x[ 0], 20, 0xe9b6c7aa);
    __md5sum_GG__(a, b, c, d, x[ 5], 5, 0xd62f105d);
    __md5sum_GG__(d, a, b, c, x[10], 9,  0x2441453);
    __md5sum_GG__(c, d, a, b, x[15], 14, 0xd8a1e681);
    __md5sum_GG__(b, c, d, a, x[ 4], 20, 0xe7d3fbc8);
    __md5sum_GG__(a, b, c, d, x[ 9], 5, 0x21e1cde6);
    __md5sum_GG__(d, a, b, c, x[14], 9, 0xc33707d6);
    __md5sum_GG__(c, d, a, b, x[ 3], 14, 0xf4d50d87);
    __md5sum_GG__(b, c, d, a, x[ 8], 20, 0x455a14ed);
    __md5sum_GG__(a, b, c, d, x[13], 5, 0xa9e3e905);
    __md5sum_GG__(d, a, b, c, x[ 2], 9, 0xfcefa3f8);
    __md5sum_GG__(c, d, a, b, x[ 7], 14, 0x676f02d9);
    __md5sum_GG__(b, c, d, a, x[12], 20, 0x8d2a4c8a);

    /* Round 3 */
    __md5sum_HH__(a, b, c, d, x[ 5], 4, 0xfffa3942);
    __md5sum_HH__(d, a, b, c, x[ 8], 11, 0x8771f681);
    __md5sum_HH__(c, d, a, b, x[11], 16, 0x6d9d6122);
    __md5sum_HH__(b, c, d, a, x[14], 23, 0xfde5380c);
    __md5sum_HH__(a, b, c, d, x[ 1], 4, 0xa4beea44);
    __md5sum_HH__(d, a, b, c, x[ 4], 11, 0x4bdecfa9);
    __md5sum_HH__(c, d, a, b, x[ 7], 16, 0xf6bb4b60);
    __md5sum_HH__(b, c, d, a, x[10], 23, 0xbebfbc70);
    __md5sum_HH__(a, b, c, d, x[13], 4, 0x289b7ec6);
    __md5sum_HH__(d, a, b, c, x[ 0], 11, 0xeaa127fa);
    __md5sum_HH__(c, d, a, b, x[ 3], 16, 0xd4ef3085);
    __md5sum_HH__(b, c, d, a, x[ 6], 23,  0x4881d05);
    __md5sum_HH__(a, b, c, d, x[ 9], 4, 0xd9d4d039);
    __md5sum_HH__(d, a, b, c, x[12], 11, 0xe6db99e5);
    __md5sum_HH__(c, d, a, b, x[15], 16, 0x1fa27cf8);
    __md5sum_HH__(b, c, d, a, x[ 2], 23, 0xc4ac5665);

    /* Round 4 */
    __md5sum_II__(a, b, c, d, x[ 0], 6, 0xf4292244);
    __md5sum_II__(d, a, b, c, x[ 7], 10, 0x432aff97);
    __md5sum_II__(c, d, a, b, x[14], 15, 0xab9423a7);
    __md5sum_II__(b, c, d, a, x[ 5], 21, 0xfc93a039);
    __md5sum_II__(a, b, c, d, x[12], 6, 0x655b59c3);
    __md5sum_II__(d, a, b, c, x[ 3], 10, 0x8f0ccc92);
    __md5sum_II__(c, d, a, b, x[10], 15, 0xffeff47d);
    __md5sum_II__(b, c, d, a, x[ 1], 21, 0x85845dd1);
    __md5sum_II__(a, b, c, d, x[ 8], 6, 0x6fa87e4f);
    __md5sum_II__(d, a, b, c, x[15], 10, 0xfe2ce6e0);
    __md5sum_II__(c, d, a, b, x[ 6], 15, 0xa3014314);
    __md5sum_II__(b, c, d, a, x[13], 21, 0x4e0811a1);
    __md5sum_II__(a, b, c, d, x[ 4], 6, 0xf7537e82);
    __md5sum_II__(d, a, b, c, x[11], 10, 0xbd3af235);
    __md5sum_II__(c, d, a, b, x[ 2], 15, 0x2ad7d2bb);
    __md5sum_II__(b, c, d, a, x[ 9], 21, 0xeb86d391);

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}


/**
 * public api
 */

static void md5sum_init(md5sum_t *ctx, uint32_t seed)
{
    memset(ctx, 0, sizeof(md5sum_t));

    ctx->state[0] = 0x67452301 + (seed * 11);
    ctx->state[1] = 0xEFCDAB89 + (seed * 71);
    ctx->state[2] = 0x98BADCFE + (seed * 37);
    ctx->state[3] = 0x10325476 + (seed * 97);
}


static void md5sum_updt (md5sum_t *ctx, const uint8_t *input, uint32_t inputlen)
{
    uint32_t i = 0, index = 0, partlen = 0;

    index = (ctx->count[0] >> 3) & 0x3F;
    partlen = 64 - index;
    ctx->count[0] += inputlen << 3;

    if(ctx->count[0] < (inputlen << 3)) {
        ctx->count[1]++;
    }
    ctx->count[1] += inputlen >> 29;

    if (inputlen >= partlen) {
        memcpy(&ctx->buffer[index], input, partlen);

        __md5sum_trans__(ctx->state, ctx->buffer);

        for (i = partlen; i+64 <= inputlen; i+=64) {
            __md5sum_trans__(ctx->state, &input[i]);
        }

        index = 0;
    } else {
        i = 0;
    }

    memcpy(&ctx->buffer[index], &input[i], inputlen - i);
}


static void md5sum_done (md5sum_t *ctx, uint8_t digest[16])
{
    uint32_t index = 0, padlen = 0;
    uint8_t bits[8];

    index = (ctx->count[0] >> 3) & 0x3F;
    padlen = (index < 56)?(56-index):(120-index);
    __md5sum_encode__(bits,ctx->count,8);

    md5sum_updt(ctx, __md5sum_padding__, padlen);
    md5sum_updt(ctx, bits, 8);

    __md5sum_encode__(digest, ctx->state, 16);
}


static const char * md5_format_lower (const uint8_t digest[16], char outbuf[MD5_SUM_LEN + 1])
{
    int i;
    char *out = outbuf;
    const uint8_t *pch =  digest;

    for (i = 0; i < 16; i++) {
        itoa(*pch++, out, 16);

        out += 2;
    }

    *out = 0;
    return outbuf;
}


static const char* md5_format_upper(const uint8_t digest[16], char outbuf[MD5_SUM_LEN + 1])
{
    int i;
    char* out = outbuf;
    const uint8_t* pch = digest;

    for (i = 0; i < 16; i++) {
        itoa(*pch++, out, 16);

        if (*out >= 'a') {
            *out -= 32;
        }
        out++;

        if (*out >= 'a') {
            *out -= 32;
        }
        out++;
    }

    *out = 0;
    return outbuf;
}


/**
 * both result and speed are same with linux:
 *
 *   md5sum $filename
 */
static int md5file (const char *pathfile, uint32_t seed, uint8_t digest[16], char chunkbuf[], uint32_t chunkbufsize)
{
    FILE * fp;

    fp = fopen(pathfile, "rb");

    if (! fp) {
        /* read file error */
        return (-1);
    } else {
        size_t rcb = 0;

        md5sum_t ctx;
        md5sum_init(&ctx, seed);

        for (;;) {
            rcb = fread(chunkbuf, 1, chunkbufsize, fp);

            if (rcb < chunkbufsize) {
                /* If an error occurs, or the end of the file is reached,
                 *   the return value is a short item count (or zero).
                 */
                if (feof(fp) && ! ferror(fp)) {
                    /* read success to end of file */
                    if (rcb != 0) {
                        md5sum_updt(&ctx, (const uint8_t *)chunkbuf, (uint32_t) rcb);
                    }

                    break;
                }

                /* read file error */
                fclose(fp);
                return (-1);
            }

            md5sum_updt(&ctx, (const uint8_t *)chunkbuf, (uint32_t) rcb);
        }

        md5sum_done(&ctx, digest);

        /* success */
        fclose(fp);
        return 0;
    }
}


#if defined(__cplusplus)
}
#endif

#endif /* MD5_SUM_H__ */
