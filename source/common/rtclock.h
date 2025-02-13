/*******************************************************************************
* Copyright © 2024-2025 Light Zhang <mapaware@hotmail.com>, MapAware, Inc.     *
* ALL RIGHTS RESERVED.                                                         *
*                                                                              *
* PERMISSION IS HEREBY GRANTED, FREE OF CHARGE, TO ANY PERSON OR ORGANIZATION  *
* OBTAINING A COPY OF THE SOFTWARE COVERED BY THIS LICENSE TO USE, REPRODUCE,  *
* DISPLAY, DISTRIBUTE, EXECUTE, AND TRANSMIT THE SOFTWARE, AND TO PREPARE      *
* DERIVATIVE WORKS OF THE SOFTWARE, AND TO PERMIT THIRD - PARTIES TO WHOM THE  *
* SOFTWARE IS FURNISHED TO DO SO, ALL SUBJECT TO THE FOLLOWING :               *
*                                                                              *
* THE COPYRIGHT NOTICES IN THE SOFTWARE AND THIS ENTIRE STATEMENT, INCLUDING   *
* THE ABOVE LICENSE GRANT, THIS RESTRICTION AND THE FOLLOWING DISCLAIMER, MUST *
* BE INCLUDED IN ALL COPIES OF THE SOFTWARE, IN WHOLE OR IN PART, AND ALL      *
* DERIVATIVE WORKS OF THE SOFTWARE, UNLESS SUCH COPIES OR DERIVATIVE WORKS ARE *
* SOLELY IN THE FORM OF MACHINE - EXECUTABLE OBJECT CODE GENERATED BY A SOURCE *
* LANGUAGE PROCESSOR.                                                          *
*                                                                              *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     *
* FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON - INFRINGEMENT.IN NO EVENT   *
* SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE    *
* FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,  *
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER  *
* DEALINGS IN THE SOFTWARE.                                                    *
*******************************************************************************/
/*
** @file      rtclock.h
**   real time clock up to year 2484.
** @copyright © 2024-2030 mapaware.top All Rights Reserved.
** @author     Liang Zhang <350137278@qq.com>
** @version 0.0.2
** @since     2019-12-20 11:15:05
** @date      2024-11-04 02:23:34
*/
#ifndef _RTCLOCK_H__
#define _RTCLOCK_H__

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

/**
 * strftime - format date and time
 *   https://linux.die.net/man/3/strftime
 */
#include <time.h>

/* +0000, +0800, -0200 */
#define TIMEZONE_FORMAT_LEN    5

static const char TIMEZONE_FORMAT_UTC[] = "+0000";


/**
 * process-wide realtime clock
 */
typedef struct _rtclock_t  *rtclock_handle;


typedef enum
{
    RTCLOCK_FREQ_SEC  = 0,    /* second */
    RTCLOCK_FREQ_MSEC = 1     /* millisecond */
} rtclock_frequency_t;


/**
 * real time clock api
 */
extern rtclock_handle rtclock_init (rtclock_frequency_t frequency);

extern void rtclock_uninit (rtclock_handle rtc);

extern int rtclock_timezone (rtclock_handle rtc, const char **tzfmt);

extern int rtclock_daylight (rtclock_handle rtc);

extern int64_t rtclock_ticktime (rtclock_handle rtc, struct timespec *ticktime);

extern void rtclock_localtime(rtclock_handle rtc, int timezone, int daylight, struct tm *tmloc, struct timespec *now);

#ifdef __cplusplus
}
#endif
#endif /* _RTCLOCK_H__ */
