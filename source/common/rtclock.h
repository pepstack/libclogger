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
