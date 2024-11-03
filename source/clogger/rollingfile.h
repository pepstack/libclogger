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
** @file rollingfile.h
**  private api for rolling file.
**
** @author     Liang Zhang <350137278@qq.com>
** @version 1.0.2
** @since      2019-12-15 16:49:10
** @date 2024-11-04 02:15:53
*/
#ifndef _ROLLINGFILE_PRIVATE_H_
#define _ROLLINGFILE_PRIVATE_H_

#if defined(__cplusplus)
extern "C"
{
#endif

#include <common/fileut.h>

#include "clogger_api.h"


typedef struct
{
    /* how to rolling file by time */
    rollingtime_t timepolicy;

    /* max size of a single file in bytes */
    ub8 maxfilesize;

    /* max files for log files in pathprefix */
    ub4 maxfilecount;

    /* path prefix like: "/var/log/applog/" */
    cstrbuf pathprefix;

    /* name prefix like: "myapp.log." */
    cstrbuf nameprefix;

    /* path file for logging: "/var/log/applog/myapp.log.0" */
    cstrbuf pathname;
    cstrbuf datesuffix;

    /* logging path file name */
    cstrbuf loggingfile;

    /* append logging file no.: 0, 1, 2, 3, ..., maxfilecount - 1 */
    int appendfileno;

    /* currently logging file handle */
    filehandle_t fhlogging;

    /* currently written bytes in logging file */
    ub8 offsetbytes;

    /* push append (not default) */
    int rollingappend;
} rollingfile_t;


/**
 * rollingfile api
 */
extern int rollingtime_from_string (const char *rotstring, int length, rollingtime_t *rot);

extern void rollingfile_apply (rollingfile_t *rof, const char *dateminfmt, int datelen);

extern void rollingfile_init (rollingfile_t *rof, const char *pathprefix, const char *nameprefix);

extern void rollingfile_uninit (rollingfile_t *rof);

extern void rollingfile_set_timepolicy (rollingfile_t *rof, rollingtime_t timepolicy);

extern void rollingfile_set_sizepolicy (rollingfile_t *rof, ub8 maxfilesize, ub4 maxfilecount, int rollingappend);

extern int rollingfile_write (rollingfile_t *rof, const char *dateminfmt, int dateminlen, const void *message, size_t msglen);

extern filehandle_t rollingfile_create (const cstrbuf pathname);

extern int rollingfile_exists (const cstrbuf pathname);

#ifdef __cplusplus
}
#endif

#endif /* _ROLLINGFILE_PRIVATE_H_ */
