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
** @file shmmaplog.c
**  shared memory map file log api.
**
** @author     Liang Zhang <350137278@qq.com>
** @version 1.0.2
** @since      2020-04-30 13:50:10
** @date 2024-11-04 03:33:26
*/
#include <common/basetype.h>

#if defined(__WINDOWS__) || defined(__CYGWIN__)
  # include <common/shmmbuf-win.h>
# else /* Linux */
  # include <common/shmmbuf.h>
#endif

#include <common/cstrbuf.h>

#include "shmmaplog.h"

static const char THIS_FILE[] = "shmmaplog.c";


typedef struct _shmmaplog_t
{
    shmmap_buffer_t *shmbuf;

#if defined(__WINDOWS__) || defined(__CYGWIN__)
    shmmbuf_semaphore_t  semaphore;

    /* Read Lock */
    HANDLE RLock;

    /* Write Lock */
    HANDLE WLock;

    /* Write Offset to the Buffer start */
    shmmbuf_state_t WOffset;

    /* Read Offset to the Buffer start */
    shmmbuf_state_t ROffset;
#endif

    char pathprefix[128];
    char shmname[0];
} shmmaplog_t;


int shmmaplog_init (const char *pathprefix, char *filename, size_t maxsizebytes, unsigned char *token, shmmaplog_hdl *pshmlog)
{
    int err;

    char shmname[256] = {0};
    int shmnamelen = 0;

#if defined(__WINDOWS__) || defined(__CYGWIN__)
    if (pathprefix) {
        if (cstr_startwith(pathprefix, cstr_length(pathprefix, 12), "/cygdrive/", 10)) {
            cstr_replace_chr(filename, '\\', '/');
            shmnamelen = snprintf_chkd_V1(shmname, sizeof(shmname), "%c:%c%.*s/%s",
                pathprefix[10],
                pathprefix[11],
                cstr_length(&pathprefix[12], 127), &pathprefix[12], filename);
        } else {
            cstr_replace_chr(filename, '\\', '/');
            shmnamelen = snprintf_chkd_V1(shmname, sizeof(shmname), "%.*s/%s", cstr_length(pathprefix, 127), pathprefix, filename);
        }
    } else {
        cstr_replace_chr(filename, '\\', '/');
        shmnamelen = snprintf_chkd_V1(shmname, sizeof(shmname), "C:/TEMP/clogger/%s", filename);
    }
#else /* Linux: default is "/dev/shm/..." */
    cstr_replace_chr(filename, '/', '-');
    cstr_replace_chr(filename, '\\', '-');
    shmnamelen = snprintf_chkd_V1(shmname, sizeof(shmname), "%s", filename);
#endif

    shmmaplog_t *shmlog = mem_alloc_zero(1, sizeof(*shmlog) + shmnamelen + 1);

    union {
        ub8token_t tkval;
        char tkbuf[8];
    } tmptoken;

    memcpy(tmptoken.tkbuf, token, sizeof(tmptoken));

    err = shmmap_buffer_create(&shmlog->shmbuf, shmname, SHMMBUF_FILEMODE_DEFAULT, maxsizebytes, &tmptoken.tkval, 0, 0);
    if (err) {
        printf("(%s:%d) shmmap_buffer_create error(%d).\n", THIS_FILE, __LINE__, err);
        mem_free(shmlog);
        return -1;
    }

    /* success */
    memcpy(shmlog->shmname, shmname, shmnamelen);
    *pshmlog = shmlog;
    return 0;
}


void shmmaplog_uninit (shmmaplog_hdl shmlog)
{
    if (shmlog && shmlog->shmbuf) {
        shmmap_buffer_close(shmlog->shmbuf);
    }
    mem_free(shmlog);
}


int shmmaplog_write (shmmaplog_hdl shmlog, const char *msg, size_t msgsz)
{
    int wok = shmmap_buffer_write(shmlog->shmbuf, (const void *) msg, msgsz);

    if (wok == SHMMBUF_WRITE_SUCCESS) {
        shmmap_buffer_post(shmlog->shmbuf, SHMMBUF_TIMEOUT_NOWAIT);
    }

    return wok;
}