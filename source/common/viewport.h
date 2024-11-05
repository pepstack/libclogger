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
** @file      viewport.h
** @brief 2D viewport transform API
**
** @author mapaware@hotmail.com
** @copyright © 2024-2030 mapaware.top All Rights Reserved.
** @version 1.0.5
**
** @since 2005-09-30 13:45:12
** @date      2024-11-04 02:42:56
**
** @note
*
*
*     ^          data       Xmax,Ymax
*  dY |-------------------------+
*     |            |            |
*     |            |            |
*     |____________|____________|
*     |            |\           |
*     |            | \          |
*     |            |  \         |
*     +----------------\------------> dX
* Xmin,Ymin             \ view
*        (Xmin,Ymin) +---\--+--------> vX
*                    |    \ |      |
*                    |     \|      |
*                    +------+------+
*                    |      |      |
*                    |      |      |
*                    |------+------+ (Xmax,Ymax)
*                    V
*                   vY
*
**
*/
#ifndef VIEWPORT_H__
#define VIEWPORT_H__

#if defined(__cplusplus)
extern "C"
{
#endif

#include "basetype.h"
#include "cgtypes.h"


typedef struct
{
    CGBox2D   dataBox;     // extent of data
    CGPoint2D dataCP;      // center point of data extent

    CGBox2D   viewBox;     // extent of view
    CGPoint2D viewCP;      // center point of view extent

    double  Xdpi;          // dot per inch in X direction
    double  dpiRatio;      // dpiRatio = Ydpi / Xdpi; Ydpi = Xdpi * dpiRatio

    // current X scale: viewPortDX/visibleDataDX  (likes: pixels/meter)
    double  XScale;

    // limits of scale
    double  MinScale;
    double  MaxScale;
} Viewport2D;


/**
 * Set new X scale factor
 */
static double ViewportSetScale(Viewport2D *vp, double newXScale)
{
    if (newXScale < vp->MinScale) {
        vp->XScale = vp->MinScale;
    } else if (newXScale > vp->MaxScale) {
        vp->XScale = vp->MaxScale;
    } else {
        vp->XScale = newXScale;
    }
    return vp->XScale;
}


/**
 *  Calculates scale and returns it
 */
static double ViewportCalcScale (Viewport2D *vp)
{
    double XScale = (vp->viewBox.Xmax - vp->viewBox.Xmin) / (vp->dataBox.Xmax - vp->dataBox.Xmin);
    double YScale = (vp->viewBox.Ymax - vp->viewBox.Ymin) / (vp->dataBox.Ymax - vp->dataBox.Ymin);
    return (XScale < YScale ? XScale : YScale);
}


/**
 * @brief init all members of Viewport2D
 */
static void ViewportInitAll(Viewport2D *vp, CGBox2D dataBox, CGBox2D viewBox, CGSize2D viewDPI, double dataPrecision)
{
    // 扩充数据范围以避免无数据溢出
    // dataPrecision 必须 > DBL_EPSILON
    CGBoxInflate(dataBox, dataPrecision);

    vp->dataBox = dataBox;
    vp->viewBox = viewBox;

    vp->dpiRatio = viewDPI.H / viewDPI.W;
    vp->Xdpi = viewDPI.W;

    vp->dataCP.X = (vp->dataBox.Xmin + vp->dataBox.Xmax) * 0.5;
    vp->dataCP.Y = (vp->dataBox.Ymin + vp->dataBox.Ymax) * 0.5;

    vp->viewCP.X = (vp->viewBox.Xmin + vp->viewBox.Xmax) * 0.5;
    vp->viewCP.Y = (vp->viewBox.Ymin + vp->viewBox.Ymax) * 0.5;

    double vx = (vp->viewBox.Xmax - vp->viewBox.Xmin);
    double vy = (vp->viewBox.Ymax - vp->viewBox.Ymin);

    double dx = (vp->dataBox.Xmax - vp->dataBox.Xmin);
    double dy = (vp->dataBox.Ymax - vp->dataBox.Ymin);

    // viewPrecision: 对应视口大小的数据坐标最小距离(精度)
    vp->MinScale = sqrt((vx * vx + vy * vy) / (dx * dx + dy * dy)) / 2;
    vp->MaxScale = sqrt((vx * vx + vy * vy) / (dataPrecision * dataPrecision)) * 2;

    ViewportSetScale(vp, ViewportCalcScale(vp));
}


/**
 * @brief Set the view port when view's size changed, such as: WM_ONSIZE
 *   (Xmin, Ymin, Xmax, Ymax) is Box of view
 */
static void ViewportResizeView(Viewport2D *vp, double Xmin, double Ymin, double Xmax, double Ymax)
{
    vp->viewBox.Xmin = Xmin;
    vp->viewBox.Ymin = Ymin;
    vp->viewBox.Xmax = Xmax;
    vp->viewBox.Ymax = Ymax;

    vp->viewCP.X = (vp->viewBox.Xmin + vp->viewBox.Xmax) * 0.5;
    vp->viewCP.Y = (vp->viewBox.Ymin + vp->viewBox.Ymax) * 0.5;
}


/**
 * Set the data extent when data changed, that is: on data loaded
 *   (Xmin, Ymin, Xmax, Ymax) are Box of data
 */
static void ViewportResetData(Viewport2D *vp, double Xmin, double Ymin, double Xmax, double Ymax)
{
    vp->dataBox.Xmin = Xmin;
    vp->dataBox.Ymin = Ymin;
    vp->dataBox.Xmax = Xmax;
    vp->dataBox.Ymax = Ymax;

    vp->dataCP.X = (vp->dataBox.Xmin + vp->dataBox.Xmax) * 0.5;
    vp->dataCP.Y = (vp->dataBox.Ymin + vp->dataBox.Ymax) * 0.5;

    ViewportSetScale(vp, ViewportCalcScale(vp));
}


/**
 * get displayed Box of data
 */
static void ViewportGetViewData(const Viewport2D *vp, CGBox2D *outDataBox)
{
    double dR;

    dR = (vp->viewBox.Xmax - vp->viewBox.Xmin) / (vp->XScale * 2);
    outDataBox->Xmin = vp->dataCP.X - dR;
    outDataBox->Xmax = vp->dataCP.X + dR;

    dR = (vp->viewBox.Ymax - vp->viewBox.Ymin) / (vp->XScale * 2);
    outDataBox->Ymin = vp->dataCP.Y - dR;
    outDataBox->Ymax = vp->dataCP.Y + dR;
}


/**
 * Get current displayed ratio: All Data Box / Viewed Data Box
 */
static CGPoint2D ViewportGetRatio(const Viewport2D *vp)
{
    CGPoint2D ratio;
    CGBox2D vwDataBox;
    ViewportGetViewData(vp, &vwDataBox);
    ratio.X = CGBoxGetDX(vp->dataBox) / CGBoxGetDX(vwDataBox);
    ratio.Y = CGBoxGetDY(vp->dataBox) / CGBoxGetDY(vwDataBox);
    return ratio;
}


/***********************************************************
 * @brief View Point to Data Point Transformation          *
 *                                                         *
 **********************************************************/
STATIC_INLINE void ViewToDataPoint(const Viewport2D *vp, const CGPoint2D *view, CGPoint2D *data)
{
   data->X = vp->dataCP.X + (view->X - vp->viewCP.X) / vp->XScale;
   data->Y = vp->dataCP.Y + (vp->viewCP.Y - view->Y / vp->dpiRatio) / vp->XScale;
}


static void ViewToDataPoints(const Viewport2D *vp, const CGPoint2D *views, CGPoint2D *datas, int count)
{
    const CGPoint2D *view = views;
    CGPoint2D *data = datas;
    while (count-- > 0) {
        ViewToDataPoint(vp, view, data);
        view++;
        data++;
    }
}


STATIC_INLINE void ViewToDataBox(const Viewport2D *vp, CGBox2D view, CGBox2D *data)
{
    CGPoint2D minPt = {view.Xmin, view.Ymax};
    CGPoint2D maxPt = {view.Xmax, view.Ymin};
    ViewToDataPoint(vp, &minPt, &data->minPt);
    ViewToDataPoint(vp, &maxPt, &data->maxPt);
}


/**
 * view length(vl) to data length(dl)
 */
#define ViewToDataLength(vp, viewLength)    ((viewLength)/(vp)->XScale)


/***********************************************************
 * @brief Data Point to View Point Transformation          *
 *                                                         *
 **********************************************************/
STATIC_INLINE void DataToViewXY(const Viewport2D *vp, double Dx, double Dy, double *Vx, double *Vy)
{
    *Vx = vp->viewCP.X + vp->XScale * (Dx - vp->dataCP.X);
    *Vy = (vp->viewCP.Y - vp->XScale * (Dy - vp->dataCP.Y)) * vp->dpiRatio;
}

STATIC_INLINE void DataToViewPoint(const Viewport2D *vp, const CGPoint2D *data, CGPoint2D *view)
{
    view->X = vp->viewCP.X + vp->XScale * (data->X - vp->dataCP.X);
    view->Y = (vp->viewCP.Y - vp->XScale * (data->Y - vp->dataCP.Y)) * vp->dpiRatio;
}

static void DataToViewPoints(const Viewport2D *vp, const CGPoint2D *datas, CGPoint2D *views, int count)
{
    const CGPoint2D *data = datas;
    CGPoint2D *view = views;
    while (count-- > 0) {
        DataToViewPoint(vp, data, view);
        data++;
        view++;
    }
}


/**
 *
 *      data                  view
 * ^         max         o------------->
 * |  --------+          |  min
 * |  |   +D  |          |   +--------|
 * |  |  Box  |    =>    |   |   Box  |
 * |  +--------          |   |   +V   |
 * | min                 |   ---------+
 *-o------------->       V           max
 */
STATIC_INLINE void DataToViewBox(const Viewport2D *vp, CGBox2D data, CGBox2D *view)
{
    CGPoint2D minPt = {data.Xmin, data.Ymax};
    CGPoint2D maxPt = {data.Xmax, data.Ymin};
    DataToViewPoint(vp, &minPt, &view->minPt);
    DataToViewPoint(vp, &maxPt, &view->maxPt);
}


/**
 * data length(vl) to view length(dl)
 */
#define DataToViewLength(vp, dataLength)    ((dataLength) * (vp)->XScale)


/***********************************************************
 * @brief            View  Manipulation                    *
 *                                                         *
 **********************************************************/

/**
 * move viewport's center to position
 */
static void ViewportCenterAt(Viewport2D *vp, double viewX, double viewY)
{
    CGPoint2D viewPt = {viewX, viewY};
    ViewToDataPoint(vp, &viewPt, &vp->dataCP);
}


/**
 * Zoom in or out view. times is fscale, must be > 0
 */
static void ViewportZoomScale(Viewport2D *vp, double newScale)
{
    ViewportSetScale(vp, vp->XScale * newScale);
}


/**
 * Just like the above. the difference is the below reset view data's center
 */
static void ViewportZoomCenter(Viewport2D *vp, double newScale)
{
    ViewportSetScale(vp, vp->XScale * newScale);
    vp->dataCP.X = (vp->dataBox.Xmax + vp->dataBox.Xmin) / 2;
    vp->dataCP.Y = (vp->dataBox.Ymax + vp->dataBox.Ymin) / 2;
}


/**
 * display all destination with times of given parameter
 */
static void ViewportZoomAll(Viewport2D *vp, double newScale)
{
    vp->dataCP.X = (vp->dataBox.Xmax + vp->dataBox.Xmin) / 2;
    vp->dataCP.Y = (vp->dataBox.Ymax + vp->dataBox.Ymin) / 2;

    ViewportSetScale(vp, ViewportCalcScale(vp) * newScale);
}


/**
 * move view, only the center of view data center changed.
 * viewOffsetX and viewOffsetY are offsets of view point
 */
static void ViewportPanView(Viewport2D *vp, double viewOffsetX, double viewOffsetY)
{
    vp->dataCP.X -= (viewOffsetX / vp->XScale);
    vp->dataCP.Y += (viewOffsetY / vp->XScale);
}


/**
 * Zoom at given viewport position Vp (X, Y)
 */
static void ViewportZoomAt(Viewport2D *vp, CGPoint2D view, double newScale)
{
    CGPoint2D data;
    ViewToDataPoint(vp, &view, &data);

    CGPoint2D view1;
    DataToViewPoint(vp, &data, &view1);

    ViewportZoomScale(vp, newScale);

    CGPoint2D view2;
    DataToViewPoint(vp, &data, &view2);

    ViewportPanView(vp, view1.X - view2.X, view1.Y - view2.Y);
}


/**
 * Zoom in (left point1 to right point2) or zoom out (right point1 to left point2) in viewport
 * pct: percentage must > 0
 */
static void ViewportZoomViewBox(Viewport2D *vp, CGPoint2D pt1, CGPoint2D pt2, float pct)
{
    double X, Y;

    int DX = (int) ((pt1.X - pt2.X) * pct + 0.5);  // DX as sign flag
    int DY = (int) ((pt1.Y - pt2.Y) * pct + 0.5);

    if (! DX || ! DY) {
        return;
    }

    // DY > 0
    DY = CG_ABS(DY);

    ViewportCenterAt(vp, (pt1.X + pt2.X)/2, (pt1.Y + pt2.Y)/2);

    if (DX > 0) {
        // zoom out
        X = DX / CGBoxGetDX(vp->viewBox) / pct;
        Y = DY / CGBoxGetDY(vp->viewBox) / pct;
    } else {
        // zoom in
        X = - CGBoxGetDX(vp->viewBox) * pct / DX;
        Y =  CGBoxGetDY(vp->viewBox) * pct / DY;
    }

    ViewportSetScale(vp, CG_MIN(X, Y) * vp->XScale);
}


static void ViewportZoomDataBox(Viewport2D *vp, CGPoint2D dataPt1, CGPoint2D dataPt2, float pct)
{
    CGPoint2D  viewPt1, viewPt2;

    DataToViewPoint(vp, &dataPt1, &viewPt1);
    DataToViewPoint(vp, &dataPt2, &viewPt2);

    ViewportZoomViewBox(vp, viewPt1, viewPt2, pct);
}

#ifdef __cplusplus
}
#endif
#endif /* VIEWPORT_H__ */
