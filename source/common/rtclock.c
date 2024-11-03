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
** @file rtclock.c
**   real time clock
**
** @author     Liang Zhang <350137278@qq.com>
** @version 0.0.2
** @since     2019-12-20 11:15:05
** @date 2024-11-04 02:23:16
*/
#include "rtclock_i.h"


static void * rtclock_timerthread (void *_rtc)
{
    int err;
    uint64_t nanos;

    struct timespec nowtime, abstime;

    rtclock_handle rtc = (rtclock_handle)_rtc;

    int64_t ratious = (int64_t) uatomic_int_get(&rtc->ratious);

    struct timespec timeout;
    timeout.tv_sec  = ratious / MICROS_OF_SECOND;
    timeout.tv_nsec = (ratious % MICROS_OF_SECOND) * 1000;

    for (;;) {
        rtclock_updatetime(rtc, &nowtime);

        nanos = nowtime.tv_nsec + timeout.tv_nsec;

        abstime.tv_sec = nowtime.tv_sec + timeout.tv_sec + nanos / NANOS_OF_SECOND;
        abstime.tv_nsec = nanos % NANOS_OF_SECOND;

        err = pthread_mutex_timedlock(&rtc->timerlock, &abstime);
        if (!err) {
            printf("rtclock_timerthread shutdown normally.\n");
            pthread_mutex_unlock(&rtc->timerlock);
            pthread_mutex_destroy(&rtc->timerlock);
            break;
        }

        if (err != ETIMEDOUT) {
            /* SHOULD NERVER RUN TO THIS! */
            if (err == EINVAL) {
                printf("[rtclock.c:%d] pthread_mutex_timedlock error EINVAL: The value specified by mutex does not refer to an initialized mutex object.\n", __LINE__);
            } else if (err == EAGAIN) {
                printf("[rtclock.c:%d] pthread_mutex_timedlock error EAGAIN: The mutex could not be acquired because the maximum number of recursive locks for mutex has been exceeded.\n", __LINE__);
            } else if (err == EDEADLK) {
                printf("[rtclock.c:%d] pthread_mutex_timedlock error EDEADLK: The current thread already owns the mutex.\n", __LINE__);
            } else {
                printf("[rtclock.c:%d] pthread_mutex_timedlock error(%d): Unexpected error.\n", __LINE__, err);
            }

            /* Place debug break point at below line to find any unexpected error! */
            exit(EXIT_FAILURE);
        }
    }

    return (void *)0;
}


rtclock_handle rtclock_init (rtclock_frequency_t frequency)
{
    int err;

    rtclock_t *rtc = mem_alloc_zero(1, sizeof(*rtc));

    if (! uatomic_int_comp_exch(&rtc->initialized, 0, 1)) {
        struct timespec nowtime;

        if (frequency) {
            /* RTCLOCK_FREQ_MSEC: 1ms = 1000 us */
            printf("[rtclock.c] rtclock_init: RTCLOCK_FREQ_MSEC\n");
            uatomic_int_set(&rtc->ratious, (int)MILLIS_OF_SECOND);
        } else {
            /* RTCLOCK_FREQ_SECOND: 1s = 1000000 us */
            printf("[rtclock.c] rtclock_init: RTCLOCK_FREQ_SECOND\n");
            uatomic_int_set(&rtc->ratious, (int)MICROS_OF_SECOND);
        }

        err = pthread_mutex_init(&rtc->timerlock, 0);
        if (err) {
            printf("[rtclock.c] pthread_mutex_init error(%d).\n", err);
            exit(EXIT_FAILURE);
        }

        err = pthread_mutex_lock(&rtc->timerlock);
        if (err) {
            printf("[rtclock.c] pthread_mutex_lock error(%d).\n", err);
            exit(EXIT_FAILURE);
        }

        rtclock_updatetime(rtc, &nowtime);

        rtc->timezone = timezone_compute(nowtime.tv_sec, rtc->timezonefmt);
        if (rtc->timezone == -1) {
            perror("[rtclock.c] timezone_compute");
            exit(EXIT_FAILURE);
        }
        rtc->daylight = daylight_compute(nowtime.tv_sec);

        if (pthread_create(&rtc->timerthread, NULL, rtclock_timerthread, (void*)rtc) != 0) {
            perror("[rtclock.c] pthread_create");
            exit(EXIT_FAILURE);
        }
    }

    return rtc;
}


void rtclock_uninit (rtclock_handle rtc)
{
    if (uatomic_int_comp_exch(&rtc->initialized, 1, 0)) {
        printf("[rtclock.c] rtclock_uninit\n");
        pthread_mutex_unlock(&rtc->timerlock);
        pthread_join(rtc->timerthread, 0);
    }

    mem_free(rtc);
}


int rtclock_daylight(rtclock_handle rtc)
{
    return (int) rtc->daylight;
}


int rtclock_timezone (rtclock_handle rtc, const char **tzfmt)
{
    if (tzfmt) {
        *tzfmt = rtc->timezonefmt;
    }
    return (int) rtc->timezone;
}


int64_t rtclock_ticktime (rtclock_handle rtc, struct timespec *ticktime)
{
    uint64_t nanos = (uint64_t)uatomic_int64_add(&rtc->ticknanos);

    ticktime->tv_sec = (time_t)(nanos / NANOS_OF_SECOND);
    ticktime->tv_nsec = nanos % NANOS_OF_SECOND;

    return (int64_t) ticktime->tv_sec;
}


void rtclock_localtime(rtclock_handle rtc, int timezone, int daylight, struct tm *tmloc, struct timespec *now)
{
    getnowtimeofday(now);

    /**
     * locale time: +0800
     * UTC time: +0000
     */
    getlocaltime_safe(tmloc, now->tv_sec, timezone, daylight);

    tmloc->tm_year += 1900;
    tmloc->tm_mon += 1;
}