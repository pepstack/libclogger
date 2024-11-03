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
** @file loggerconf.h
**  clogger config definitions.
**
** @author     Liang Zhang <350137278@qq.com>
** @version 1.0.2
** @since      2019-12-17 21:53:05
** @date 2024-11-04 00:00:42
*/
#ifndef LOGGERCONF_H_INCLUDED
#define LOGGERCONF_H_INCLUDED

#if defined(__cplusplus)
extern "C"
{
#endif

#include "common/readconf.h"

#include "clogger_api.h"
#include "rollingfile.h"
#include "shmmaplog.h"


typedef struct _logger_conf_t
{
    int           loggerid;

    ub4           magickey;

    int           maxconcurrents;
    int           maxmsgsize;
    int           queuelength;
    int           appender;

    ub8           maxfilesize;
    ub4           maxfilecount;
    int           rollingappend;

    int           timeunit;
    int           loctime;
    int           colorstyle;
    int           timestampid;

    int           filelineno;
    int           function;
    int           autowrapline;
    int           hideident;

#ifndef CLOGGER_NO_THREADNO
    int           processid;
    int           threadno;
#endif

    cstrbuf       ident;
    cstrbuf       pathprefix;
    cstrbuf       nameprefix;
    cstrbuf       shmlogfile;
    cstrbuf       winsyslogconf;

    clog_level_t       loglevel;
    clog_layout_t      layout;
    clog_dateformat_t  dateformat;

    rollingtime_t      rollingtime;

    char errmsg[CLOG_ERRMSG_LEN_MAX + 1];
} logger_conf_t;


#ifdef __cplusplus
}
#endif

#endif /* LOGGERCONF_H_INCLUDED */