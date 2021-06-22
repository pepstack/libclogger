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
 * @filename   loggermgr_i.h
 *  clogger manager methods.
 *
 * @author     Liang Zhang <350137278@qq.com>
 * @version    0.0.1
 * @create     2019-12-17 21:53:05
 * @update     2020-12-14 18:53:05
 */
#ifndef _LOGGERMGR_I_H_
#define _LOGGERMGR_I_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "loggerconf.h"

#include <common/mscrtdbg.h>
#include <common/rtclock_i.h>
#include <common/unsema.h>
#include <common/uatomic.h>
#include <common/thread_rwlock.h>
#include <common/ringbufst.h>
#include <common/ringbuf.h>
#include <common/emerglog.h>
#include <common/shmipc.h>
#include <common/md5sum.h>
#include <common/smallregex.h>   /* https://gitlab.com/relkom/small-regex */
#include <common/uthash_incl.h>

/* using pthread or pthread-w32 */
#include <sched.h>
#include <pthread.h>

#if defined(__WINDOWS__)
    //# include <signal.h>
    # include <common/win32/syslog.h>

    # if !defined(__MINGW__)
    #   pragma comment(lib, "pthreadVC2.lib")
    # endif
#else
    # include <syslog.h>
#endif


/**
 * version number(2.1.2.3) must update once libclogger codes has been revised !
 */
#define CLOGGER_MAJOR_VERSION    0x0201   /* 2.1 */
#define CLOGGER_MINOR_VERSION    0x0203   /* 2.3 */

#define CLOG_LOGGERID_MAX        255


struct clogger_version_mgr_t
{
    struct {
        ub2 major_version;
        ub2 minor_version;
    };

    struct {
        char build_date[14];
        char build_time[10];
    };

    int pid;

    uatomic_ptr pvmgr;
};


struct logger_manager_t
{
    /* 0: not initialized, 1 - initialized */
    uatomic_int initialized;

    /* cached app logger if not null */
    clog_logger app_logger;

#ifdef DISABLE_THREAD_RWLOCK
    pthread_mutex_t thrlock;
#else
    ThreadRWLock_t rwlock;
#endif

    rtclock_handle rtclock;

    /* logger by id */
    clog_logger idloggers[CLOG_LOGGERID_MAX + 1];

    cstrbuf cfgfile;
    struct clogger_ident_t *loggers;
};


struct clogger_ident_t
{
    clog_logger logger;

    /* makes this structure hashable */
    UT_hash_handle hh;

    /* key name for clogger */
    int idlen;
    char ident[0];
};

#ifdef __cplusplus
}
#endif

#endif /* _LOGGERMGR_I_H_ */
