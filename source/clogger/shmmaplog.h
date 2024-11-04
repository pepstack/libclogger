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
** @file      shmmaplog.h
**  shared memory map log api.
**
** @author     Liang Zhang <350137278@qq.com>
** @version 1.0.2
** @since      2020-04-30 13:50:10
** @date      2024-11-04 00:08:01
*/
#ifndef _SHMMAPLOG_PRIVATE_H_
#define _SHMMAPLOG_PRIVATE_H_

#if defined(__cplusplus)
extern "C"
{
#endif

#include <stddef.h>

typedef struct _shmmaplog_t * shmmaplog_hdl;

extern int shmmaplog_init (const char *pathprefix, char *filename, size_t maxsizebytes, unsigned char *token /* at least 8 chars */, shmmaplog_hdl *pshmlog);

extern void shmmaplog_uninit (shmmaplog_hdl shmlog);

extern int shmmaplog_write (shmmaplog_hdl shmlog, const char *msg, size_t msgsz);

#ifdef __cplusplus
}
#endif

#endif /* _SHMMAPLOG_PRIVATE_H_ */
