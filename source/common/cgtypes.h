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
/**
 * @file cgtypes.h
 * @brief C Gemoetry Types Definitions
 *
 * @author mapaware@hotmail.com
 * @copyright © 2024-2030 mapaware.top All Rights Reserved.
 * @version 0.1.3
 *
 * @since 2024-10-16 12:45:30
 * @date 2024-10-16 22:09:07
 *
 * @note
 */
#ifndef GEOMTYPE_H__
#define GEOMTYPE_H__

#if defined(__cplusplus)
extern "C"
{
#endif

#include <stdint.h>
#include <math.h>
#include <float.h>
#include <memory.h>


typedef struct
{
    double X;
    double Y;
} CGPoint2D;


typedef struct
{
    int64_t X;
    int64_t Y;
} CGPoint2L;


typedef struct
{
    double W;   // Width
    double H;   // Height
} CGSize2D;


typedef struct
{
    union {
        struct {
            double Xmin;
            double Ymin;
        };
        CGPoint2D minPt;
    };

    union {
        struct {
            double Xmax;
            double Ymax;
        };
        CGPoint2D maxPt;
    };
} CGBox2D;


typedef struct
{
    union {
        struct {
            int64_t Xmin;
            int64_t Ymin;
        };
        CGPoint2L minPt;
    };

    union {
        struct {
            int64_t Xmax;
            int64_t Ymax;
        };
        CGPoint2L maxPt;
    };
} CGBox2L;


#define CG_ABS(v)       ((v) < 0? -(v):(v))
#define CG_MIN(a, b)    ((a) < (b) ? (a) : (b))
#define CG_MAX(a, b)    ((a) > (b) ? (a) : (b))

#define CGPointNotEqual(X1, Y1, X2, Y2, precision)   (X1-X2>precision || Y1-Y2>precision || X2-X1>precision || Y2-Y1>precision)


#define CGBoxGetDX(box)   (box.Xmax - box.Xmin)
#define CGBoxGetDY(box)   (box.Ymax - box.Ymin)

#define CGBoxIsOverlap(a, b)   ((a).Xmin<(b).Xmax && (a).Ymin<(b).Ymax && (b).Xmin<(a).Xmax && (b).Ymin<(a).Ymax)

#define CGBoxInflate(box, d)   do { box.Xmin -= d; box.Ymin -= d; box.Xmax += d; box.Ymax += d; } while(0)

#ifdef __cplusplus
}
#endif
#endif /* GEOMTYPE_H__ */
