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
** @file      rtclock_i.h
**   real time clock up to year 2484.
**   internal type definition
** @copyright © 2024-2030 mapaware.top All Rights Reserved.
** @author     Liang Zhang <350137278@qq.com>
** @version   0.0.5
** @since     2020-12-14 10:44:05
** @date     2024-11-04 01:44:05
*/
#ifndef _RTCLOCK_I_H__
#define _RTCLOCK_I_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rtclock.h"
#include "uatomic.h"

#include "timeut.h"
#include "fileut.h"


/**
 * linux date command:
 *   https://blog.csdn.net/mergerly/article/details/41597235
 */

#ifdef _WIN32
  # define HAVE_STRUCT_TIMESPEC
  # include <pthread.h>

  # ifdef _MSC_VER
    # pragma comment(lib, "pthreadVC2.lib")  // link to pthread-w32 lib
  # endif
# else
  # include <pthread.h>
#endif

/* (ms) milliseconds of one second */
#define MILLIS_OF_SECOND   ((uint64_t)1000l)

/* (us) microseconds of one second */
#define MICROS_OF_SECOND   ((uint64_t)1000000l)

/* (ns) nanoeconds of one second */
#define NANOS_OF_SECOND    ((uint64_t)1000000000l)


typedef struct _rtclock_t
{
    /* if 0 : no initialized */
    uatomic_int initialized;

    uatomic_int ratious;

    pthread_mutex_t timerlock;
    pthread_t timerthread;

    /* fake time in nanos supports up to max year 2484 AC. */
    uatomic_int64 ticknanos;

    int daylight;
    long timezone;
    char timezonefmt[TIMEZONE_FORMAT_LEN + 1];
} rtclock_t;


static void rtclock_updatetime (rtclock_handle rtc, struct timespec *nowtime)
{
    uint64_t timeus;

    getnowtimeofday(nowtime);

    /* now time in us: timeus=1606890292682864 */
    timeus = (uint64_t)(((int64_t) nowtime->tv_sec) * MICROS_OF_SECOND + nowtime->tv_nsec / 1000);

    /* aligned time in us: timeus=1606890292000000 */
    timeus = (uint64_t)((timeus / rtc->ratious) * rtc->ratious);

    /* set aligned time in ns: ticknanos=1606890292000000000 */
    uatomic_int64_set(&rtc->ticknanos, timeus * (uint64_t)1000);
}


#ifdef __cplusplus
}
#endif
#endif /* _RTCLOCK_I_H__ */
