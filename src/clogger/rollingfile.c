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
 * @filename   rollingfile.c
 *  rolling file for logging
 *
 * @author     Liang Zhang <350137278@qq.com>
 * @version    1.0.0
 * @create     2019-12-26 16:49:10
 * @update     2019-12-26 23:19:22
 */
#include "rollingfile.h"

static const char THIS_FILE[] = "rollingfile.c";


int rollingtime_from_string (const char *rotstring, int length, rollingtime_t *rot)
{
    static const char * rots[] = {
        "min", "1min",
        "5m",  "5min",
        "10m", "10min",
        "30m", "30min",
        "hour",
        "day",
        "mon",
        "year"
    };

    static const rollingtime_t roti[] = {
        ROLLING_TM_MIN_1,  ROLLING_TM_MIN_1,
        ROLLING_TM_MIN_5,  ROLLING_TM_MIN_5,
        ROLLING_TM_MIN_10, ROLLING_TM_MIN_10,
        ROLLING_TM_MIN_30, ROLLING_TM_MIN_30,
        ROLLING_TM_HOUR,
        ROLLING_TM_DAY,
        ROLLING_TM_MON,
        ROLLING_TM_YEAR
    };

    int i;
    for (i = 0; i < sizeof(rots)/sizeof(rots[0]); i++) {
        if (!cstr_compare_len(rotstring, length, rots[i], cstr_length(rots[i], 8), 1)) {
            /* success */
            *rot = roti[i];
            return 1;
        }
    }

    /* failed as default */
    return 0;
}


static void rollingfile_update (rollingfile_t *rof)
{
    file_close(&rof->fhlogging);

    if (rof->rollingappend) {
        rof->appendfileno = (rof->appendfileno + 1) % rof->maxfilecount;

        cstrbuf nextloggingfile = cstrbufNew(rof->loggingfile->len + 20, rof->loggingfile->str, rof->loggingfile->len);

        if (rof->appendfileno > 0) {
            nextloggingfile = cstrbufCat(nextloggingfile, ".%d", rof->appendfileno);
        }

        /* remove and create next log file */
        pathfile_remove(nextloggingfile->str);

        rof->fhlogging = rollingfile_create(nextloggingfile);
        rof->offsetbytes = 0;

        cstrbufFree(&nextloggingfile);
    } else {
        int i = (int) rof->maxfilecount;

        while (i-- > 1) {
            cstrbuf filefrom = (i > 1? cstrbufCat(0, "%.*s.%d", cstrbufGetLen(rof->loggingfile), cstrbufGetStr(rof->loggingfile), i-1) : rof->loggingfile);

            if (rollingfile_exists(filefrom)) {
                cstrbuf fileto = cstrbufCat(0, "%.*s.%d", cstrbufGetLen(rof->loggingfile), cstrbufGetStr(rof->loggingfile), i);
                if (rollingfile_exists(fileto)) {
                    pathfile_remove(fileto->str);
                }
                pathfile_move(filefrom->str, fileto->str);
                cstrbufFree(&fileto);
            }

            if (filefrom != rof->loggingfile) {
                cstrbufFree(&filefrom);
            }
        }

        /* remove and create log file */
        pathfile_remove(rof->loggingfile->str);

        rof->fhlogging = rollingfile_create(rof->loggingfile);
        rof->offsetbytes = 0;
    }
}


/* check and rolling file name */
void rollingfile_apply (rollingfile_t *rof, const char *dateminfmt, int datelen)
{
    if (datelen) {
        /* timepolicy valid */
        if (! rof->loggingfile || ! cstr_startwith(rof->loggingfile->str + rof->pathname->len, rof->loggingfile->len - rof->pathname->len, dateminfmt, datelen)) {
            ub4 pathlen = 0;

            /* first time make full name of path file OR rolling log file */
            size_t pathsize = rof->pathname->len + datelen + (rof->datesuffix? rof->datesuffix->len : 0) + 20;
            char * pathfile = (char *) alloca(pathsize);

            pathlen = cstrbufCopyTo(rof->pathname, pathfile, 0);

            memcpy(pathfile + pathlen, dateminfmt, datelen);
            pathlen += datelen;

            pathlen = cstrbufCopyTo(rof->datesuffix, pathfile, pathlen);
            pathfile[pathlen] = 0;

            if (! rof->loggingfile) {
                rof->loggingfile = cstrbufNew((ub4)pathsize, pathfile, (ub4)pathlen);
            } else {
                file_close(&rof->fhlogging);
                rof->loggingfile = cstrbufDup(rof->loggingfile, pathfile,(ub4) pathlen);
            }

            rof->fhlogging = rollingfile_create(rof->loggingfile);
            rof->offsetbytes = 0;

            if (rof->fhlogging == filehandle_invalid && rollingfile_exists(rof->loggingfile)) {
                rollingfile_update(rof);
            }
        }
    } else {
        /* sizepolicy */
        if (! rof->loggingfile) {
            ub4 pathlen = 0;

            size_t pathsize = rof->pathname->len + 1 + (rof->datesuffix? rof->datesuffix->len : 0) + 20;

            char * pathfile = (char *) alloca(pathsize);

            pathlen = cstrbufCopyTo(rof->pathname, pathfile, 0);
            if (rof->datesuffix) {
                pathfile[pathlen++] = '0';
                pathlen = cstrbufCopyTo(rof->datesuffix, pathfile, pathlen);
            }
            pathfile[pathlen] = 0;

            rof->loggingfile = cstrbufNew((ub4)pathsize, pathfile, pathlen);

            rof->fhlogging = rollingfile_create(rof->loggingfile);
            rof->offsetbytes = 0;

            if (rof->fhlogging == filehandle_invalid && rollingfile_exists(rof->loggingfile)) {
                rollingfile_update(rof);
            }
        }
    }

    /* check rolling size */
    if (rof->offsetbytes >= rof->maxfilesize) {
        rollingfile_update(rof);
    }
}


void rollingfile_init (rollingfile_t *rof, const char *pathprefix, const char *nameprefix)
{
    static char pathsep[] = {PATH_SEPARATOR_CHAR, '\0'};

    rof->pathprefix = cstrbufNew(ROF_PATHPREFIX_LEN_MAX, pathprefix, -1);
    rof->nameprefix = cstrbufNew(ROF_NAMEPATTERN_LEN_MAX, nameprefix, -1);

    if (! cstr_endwith(rof->pathprefix->str, rof->pathprefix->len, pathsep, 1) &&
        ! cstr_endwith(rof->pathprefix->str, rof->pathprefix->len, "/", 1)) {
        rof->pathprefix = cstrbufCat(rof->pathprefix, pathsep);
    }

    if (cstr_containwith(rof->nameprefix->str, rof->nameprefix->len, ROF_DATE_SYMBOL, ROF_DATE_SYMBOLLEN) != -1) {
        rof->datesuffix = cstrbufNew(ROF_NAMEPATTERN_LEN_MAX, strstr(rof->nameprefix->str, ROF_DATE_SYMBOL) + ROF_DATE_SYMBOLLEN, -1);

        rof->pathname = cstrbufCat(NULL, "%.*s%.*s", rof->pathprefix->len, rof->pathprefix->str,
                            rof->nameprefix->len  - rof->datesuffix->len - ROF_DATE_SYMBOLLEN, rof->nameprefix->str);
    } else {
        rof->datesuffix = NULL;
        rof->pathname = cstrbufCat(NULL, "%.*s%.*s", rof->pathprefix->len, rof->pathprefix->str, rof->nameprefix->len, rof->nameprefix->str);
    }
}


void rollingfile_uninit (rollingfile_t *rof)
{
    file_close(&rof->fhlogging);

    cstrbufFree(&rof->pathprefix);
    cstrbufFree(&rof->nameprefix);
    cstrbufFree(&rof->datesuffix);
    cstrbufFree(&rof->pathname);
    cstrbufFree(&rof->loggingfile);
}


void rollingfile_set_timepolicy (rollingfile_t *rof, rollingtime_t timepolicy)
{
    rof->timepolicy = timepolicy;
}


void rollingfile_set_sizepolicy (rollingfile_t *rof, ub8 maxfilesize, ub4 maxfilecount, int rollingappend)
{
    CHKCONFIG_INT_VALUE(10485760, 1048576, ROF_MAXFILESIZE, maxfilesize);
    CHKCONFIG_INT_VALUE(10, 1, ROF_MAXFILECOUNT, maxfilecount);

    rof->maxfilesize = maxfilesize;
    rof->maxfilecount = maxfilecount;

    rof->rollingappend = (rollingappend? 1 : 0);
    rof->appendfileno = 0;
}


int rollingfile_write (rollingfile_t *rof, const char *dateminfmt, int dateminlen, const void *message, size_t msglen)
{
    int err;

    rollingfile_apply(rof, dateminfmt, dateminlen);

    err = file_writebytes(rof->fhlogging, (const char *) message, (ub4)msglen);

    if (!err) {
        rof->offsetbytes += msglen;
    }

    return err;
}


filehandle_t rollingfile_create(const cstrbuf pathname)
{
#if defined(_WIN32)
    return file_create(pathname->str, GENERIC_WRITE, FILE_ATTRIBUTE_NORMAL);
#else
    /* mode = S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH (0644) */
    return file_create(pathname->str, O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
#endif
}


int rollingfile_exists (const cstrbuf pathname)
{
    return pathfile_exists(cstrbufGetStr(pathname));
}
