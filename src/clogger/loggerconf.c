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
 * @filename   loggerconf.c
 *  clogger config api.
 *
 * @author     Liang Zhang <350137278@qq.com>
 * @version    0.0.1
 * @create     2019-12-17 21:53:05
 * @update     2019-12-27 17:16:05
 */
#include "loggerconf.h"

static const char THIS_FILE[] = "loggerconf.c";

#define CLOGGER_SECTION_IDENTS_MAX    512
#define CLOGGER_SHMLOG_FILEPATTERN    "SHMLOG/clogger-<IDENT>.shmmbuf"


static int split_string_chkd (const char * str, int len, char delim, char **outstrs, int outstrslen[], int maxoutnum)
{
    char *p;
    const char *s = str;

    int outlen;
    int i = 0;

    int n = 1;
    while (s && (p = (char *)strchr(s, delim)) && (p < str +len)) {
        s = p+1;
        n++;
    }

    if (! outstrs) {
        // only to get count
        return n;
    }

    if (n > 0) {
        char *sb;
        char *s0 = (char*) mem_alloc_unset(len + 1);
        memcpy(s0, str, len);
        s0[len] = 0;
        sb = s0;
        while (sb && (p = strchr(sb, delim))) {
            *p++ = 0;
            if (i < maxoutnum) {
                // remove whitespaces
                outlen = 0;
                outstrs[i] = mem_strdup( cstr_LRtrim_chr(sb, 32, &outlen) );
                if (outstrslen) {
                    outstrslen[i] = outlen;
                }
                i++;
            } else {
                // overflow than maxoutnum
                break;
            }
            sb = (char *) p;
        }

        if (i < maxoutnum) {
            outlen = 0;
            outstrs[i] = mem_strdup( cstr_LRtrim_chr(sb, 32, &outlen) );
            if (outstrslen) {
                outstrslen[i] = outlen;
            }
            i++;
        }
        mem_free(s0);
    }
    return i;
}


void logger_conf_init_default (clogger_conf conf, const char *ident, const char *pathprefix, const char *winsyslogconf)
{
    bzero(conf, sizeof(*conf));

    conf->ident = cstrbufNew(0, ident, -1);

    /* NOTES: default magic key is from my QQ email: 350137278@qq.com */
    conf->magickey = 350137278;

    conf->maxmsgsize =  CLOG_MSGBUF_SIZE_DEFAULT;
    conf->queuelength = 512;
    conf->maxconcurrents = 128;

    conf->appender = CLOG_APPENDER_STDOUT;

    conf->pathprefix = cstrbufNew(0, pathprefix, -1);
    conf->nameprefix = cstrbufNew(ROF_NAMEPATTERN_LEN_MAX + 8, CLOG_NAMEPATTERN_DEFAULT, -1);
    conf->shmlogfile = cstrbufNew(0, CLOGGER_SHMLOG_FILEPATTERN, -1);

    if (! winsyslogconf) {
        conf->winsyslogconf = cstrbufNew(0, "localhost:514", -1);
    } else {
        conf->winsyslogconf = cstrbufNew(0, winsyslogconf, -1);
    }

    conf->rollingtime = ROLLING_TM_NONE;

    conf->maxfilesize = 16777216;
    conf->maxfilecount = 10;

    conf->loglevel = CLOG_LEVEL_DEBUG;
    conf->layout = CLOG_LAYOUT_DATED;
    conf->dateformat = CLOG_DATEFMT_RFC_3339;

    conf->timeunit = CLOG_TIMEUNIT_SEC;
    conf->loctime = 0;

    conf->colorstyle = 0;
    conf->timestampid = 0;
}


void logger_conf_final_release (clogger_conf conf)
{
    cstrbufFree(&conf->ident);
    cstrbufFree(&conf->pathprefix);
    cstrbufFree(&conf->nameprefix);
    cstrbufFree(&conf->shmlogfile);
    cstrbufFree(&conf->winsyslogconf);
}


int logger_conf_get_creatflags (const clogger_conf conf)
{
    return (int) (conf->colorstyle |
        conf->timestampid |
        conf->loctime |
        conf->timeunit |
        conf->appender |
        conf->filelineno |
        conf->function);
}


int logger_conf_load_config (const char *cfgfile, const char *ident, clogger_conf conf)
{
    int i, j, ncb, secs;
    void *seclist;

    int loaderror = -1;

    cstrbuf rollingpolicy = 0;

    char readbuf[READCONF_MAX_LINESIZE];

    int identlen = cstr_length(ident, ROF_NAMEPATTERN_LEN_MAX);

    secs = ConfGetSectionList(cfgfile, &seclist);
    if (secs == -1) {
        snprintf(conf->errmsg, CLOG_ERRMSG_LEN_MAX, "config file not found <%s>", cfgfile);
        return loaderror;
    }
    if (secs == 0) {
        snprintf(conf->errmsg, CLOG_ERRMSG_LEN_MAX, "no section in config file <%s>", cfgfile);
        return loaderror;
    }

    // [clogger:$ident]
    for (i = 0; i < secs; ++i) {
        char *sec, *family, *qualifier;

        sec = ConfSectionListGetAt(seclist, i);

        if (ConfSectionParse(sec, &family, &qualifier) == 2) {
            if (!cstr_compare_len("clogger", 7, family, -1, 0)) {
                // qualifier = "ident1 ident2 ..."
                char *idents[CLOGGER_SECTION_IDENTS_MAX] = {0};
                int identslen[CLOGGER_SECTION_IDENTS_MAX] = {0};
                int numidents = cstr_split_multi_chrs(qualifier, cstr_length(qualifier, READCONF_MAX_LINESIZE), " ,;|", 4, idents, identslen, CLOGGER_SECTION_IDENTS_MAX);

                for (j = 0; j < numidents; j++) {
                    if (!cstr_compare_len(ident, identlen, idents[j], identslen[j], 0)) {
                        loaderror = 1;

                        ncb = ConfReadValueParsed(cfgfile, family, qualifier, "magickey", readbuf, sizeof(readbuf));
                        if ( ncb > 1 ) {
                            conf->magickey = (ub4) strtol(readbuf, 0, 10);
                        }

                        ncb = ConfReadValueParsed(cfgfile, family, qualifier, "maxmsgsize", readbuf, sizeof(readbuf));
                        if ( ncb > 1 ) {
                            conf->maxmsgsize = (int) strtol(readbuf, 0, 10);
                            conf->maxmsgsize = memapi_align_psize(conf->maxmsgsize);
                        }

                        ncb = ConfReadValueParsed(cfgfile, family, qualifier, "queuelength", readbuf, sizeof(readbuf));
                        if ( ncb > 1 ) {
                            conf->queuelength = (int) strtol(readbuf, 0, 10);
                            conf->maxconcurrents = memapi_align_psize(conf->queuelength / 4);
                        }

                        ncb = ConfReadValueParsed(cfgfile, family, qualifier, "appender", readbuf, sizeof(readbuf));
                        if ( ncb-- > 1 ) {
                            clog_appender_from_string(readbuf, ncb, &conf->appender);
                        }

                        ncb = ConfReadValueParsed(cfgfile, family, qualifier, "pathprefix", readbuf, sizeof(readbuf));
                        if ( ncb-- > 1 ) {
                            conf->pathprefix = cstrbufDup(conf->pathprefix, readbuf, (ncb > 255 ? 255 : ncb));
                        }

                        ncb = ConfReadValueParsed(cfgfile, family, qualifier, "nameprefix", readbuf, sizeof(readbuf));
                        if ( ncb-- > 1 ) {
                            conf->nameprefix = cstrbufDup(conf->nameprefix, readbuf, (ncb > 127 ? 127 : ncb));
                        }

                        ncb = ConfReadValueParsed(cfgfile, family, qualifier, "shmlogfile", readbuf, sizeof(readbuf));
                        if ( ncb-- > 1 ) {
                            conf->shmlogfile = cstrbufDup(conf->shmlogfile, readbuf, (ncb > 127 ? 127 : ncb));
                        }

                        ncb = ConfReadValueParsed(cfgfile, family, qualifier, "rollingpolicy", readbuf, sizeof(readbuf));
                        if ( ncb-- > 1 ) {
                            rollingpolicy = cstrbufDup(rollingpolicy, readbuf, (ncb > 127 ? 127 : ncb));
                        }

                        ncb = ConfReadValueParsed(cfgfile, family, qualifier, "loglevel", readbuf, sizeof(readbuf));
                        if ( ncb-- > 1 ) {
                            clog_level_from_string(readbuf, ncb, &conf->loglevel);
                        }

                        ncb = ConfReadValueParsed(cfgfile, family, qualifier, "layout", readbuf, sizeof(readbuf));
                        if ( ncb-- > 1 ) {
                            clog_layout_from_string(readbuf, ncb, &conf->layout);
                        }

                        ncb = ConfReadValueParsed(cfgfile, family, qualifier, "dateformat", readbuf, sizeof(readbuf));
                        if ( ncb-- > 1 ) {
                            clog_dateformat_from_string(readbuf, ncb, &conf->dateformat);
                        }

                        ncb = ConfReadValueParsed(cfgfile, family, qualifier, "timeunit", readbuf, sizeof(readbuf));
                        if ( ncb-- > 1 ) {
                            if (!cstr_compare_len(readbuf, ncb, "s", 1, 1)) {
                                conf->timeunit = CLOG_TIMEUNIT_SEC;
                            } else if (!cstr_compare_len(readbuf, ncb, "ms", 2, 1)) {
                                conf->timeunit = CLOG_TIMEUNIT_MSEC;
                            } else if (!cstr_compare_len(readbuf, ncb, "us", 2, 1)) {
                                conf->timeunit = CLOG_TIMEUNIT_USEC;
                            }
                        }

                        ncb = ConfReadValueParsed(cfgfile, family, qualifier, "autowrapline", readbuf, sizeof(readbuf));
                        if ( ncb ) {
                            if (ConfParseBoolValue(readbuf, 1)) {
                                conf->autowrapline = 1;
                            }
                        }

#ifndef CLOGGER_NO_THREADNO
                        ncb = ConfReadValueParsed(cfgfile, family, qualifier, "processid", readbuf, sizeof(readbuf));
                        if ( ncb ) {
                            if (ConfParseBoolValue(readbuf, 1)) {
                                conf->processid = 1;
                            }
                        }

                        ncb = ConfReadValueParsed(cfgfile, family, qualifier, "threadno", readbuf, sizeof(readbuf));
                        if ( ncb ) {
                            if (ConfParseBoolValue(readbuf, 1)) {
                                conf->threadno = 1;

                                if (!conf->processid) {
                                    conf->processid = 1;
                                }
                            }
                        }
#endif

                        ncb = ConfReadValueParsed(cfgfile, family, qualifier, "hideident", readbuf, sizeof(readbuf));
                        if ( ncb ) {
                            if (ConfParseBoolValue(readbuf, 1)) {
                                conf->hideident = 1;
                            }
                        }

                        ncb = ConfReadValueParsed(cfgfile, family, qualifier, "timestampid", readbuf, sizeof(readbuf));
                        if ( ncb ) {
                            if (ConfParseBoolValue(readbuf, 1)) {
                                conf->timestampid = CLOG_TIMESTAMP_ID;
                            }
                        }

                        ncb = ConfReadValueParsed(cfgfile, family, qualifier, "localtime", readbuf, sizeof(readbuf));
                        if ( ncb ) {
                            if (ConfParseBoolValue(readbuf, 1)) {
                                conf->loctime = CLOG_TIMEZONE_LOC;
                            }
                        }

                        ncb = ConfReadValueParsed(cfgfile, family, qualifier, "colorstyle", readbuf, sizeof(readbuf));
                        if ( ncb ) {
                            if (ConfParseBoolValue(readbuf, 1)) {
                                conf->colorstyle = CLOG_LEVEL_COLORS | CLOG_LEVEL_STYLES;
                            }
                        }

                        ncb = ConfReadValueParsed(cfgfile, family, qualifier, "filelineno", readbuf, sizeof(readbuf));
                        if ( ncb ) {
                            if (ConfParseBoolValue(readbuf, 1)) {
                                conf->filelineno = CLOG_FILE_LINENO;
                            }
                        }

                        ncb = ConfReadValueParsed(cfgfile, family, qualifier, "function", readbuf, sizeof(readbuf));
                        if ( ncb ) {
                            if (ConfParseBoolValue(readbuf, 1)) {
                                conf->function = CLOG_FUNCTION_NAME;
                            }
                        }

                        do {
                            char *valuebuf = 0;

                            if ( ConfReadValueParsedAlloc(cfgfile, family, qualifier, "enableflags", &valuebuf) ) {
                                // autowrapline, timestampid, localtime, colorstyle, filelineno, function, hideident
                                char *keynames[10] = {0};
                                int keyslen[10] = {0};

                                int numkeys = split_string_chkd(valuebuf, cstr_length(valuebuf, 255), ',', keynames, keyslen, sizeof(keyslen)/sizeof(keyslen[0]));

                                if (numkeys > 0) {
                                    if (cstr_findstr_in("autowrapline", cstr_length("autowrapline", 20), (const char **)keynames, numkeys, 1) != -1) {
                                        conf->autowrapline = 1;
                                    }
                                    if (cstr_findstr_in("hideident", cstr_length("hideident", 20), (const char **)keynames, numkeys, 1) != -1) {
                                        conf->hideident = 1;
                                    }
                                    if (cstr_findstr_in("timestampid", cstr_length("timestampid", 20), (const char **)keynames, numkeys, 1) != -1) {
                                        conf->timestampid = CLOG_TIMESTAMP_ID;
                                    }
                                    if (cstr_findstr_in("localtime", cstr_length("localtime", 20), (const char **)keynames, numkeys, 1) != -1) {
                                        conf->loctime = CLOG_TIMEZONE_LOC;
                                    }
                                    if (cstr_findstr_in("colorstyle", cstr_length("colorstyle", 20), (const char **)keynames, numkeys, 1) != -1) {
                                        conf->colorstyle = CLOG_LEVEL_COLORS | CLOG_LEVEL_STYLES;
                                    }
                                    if (cstr_findstr_in("filelineno", cstr_length("filelineno", 20), (const char **)keynames, numkeys, 1) != -1) {
                                        conf->filelineno = CLOG_FILE_LINENO;
                                    }
                                    if (cstr_findstr_in("function", cstr_length("function", 20), (const char **)keynames, numkeys, 1) != -1) {
                                        conf->function = CLOG_FUNCTION_NAME;
                                    }
#ifndef CLOGGER_NO_THREADNO
                                    if (cstr_findstr_in("processid", cstr_length("processid", 20), (const char **)keynames, numkeys, 1) != -1) {
                                        conf->processid = 1;
                                    }
                                    if (cstr_findstr_in("threadno", cstr_length("threadno", 20), (const char **)keynames, numkeys, 1) != -1) {
                                        conf->threadno = 1;
                                        if (! conf->processid) {
                                            conf->processid = 1;
                                        }
                                    }
#endif
                                    cstr_varray_free(keynames, numkeys);
                                }
                            }

                            ConfMemFree(valuebuf);
                        } while(0);

                        cstr_varray_free(idents, numidents);
                        goto found_ident;
                    }
                }

                cstr_varray_free(idents, numidents);
            }
        }
    }
found_ident:
    ConfSectionListFree(seclist);

    if (loaderror != 1) {
        snprintf(conf->errmsg, CLOG_ERRMSG_LEN_MAX, "not found section: [clogger:%s]", ident);
        cstrbufFree(&rollingpolicy);
        return loaderror;
    }

    loaderror = 0;

    // [rollingpolicy:$policy]
    if (rollingpolicy) {
        loaderror = 1;
        snprintf(conf->errmsg, CLOG_ERRMSG_LEN_MAX, "not found rollingpolicy: [rollingpolicy:%.*s]", (int)rollingpolicy->len, rollingpolicy->str);

        secs = ConfGetSectionList(cfgfile, &seclist);

        for (i = 0; i < secs; ++i) {
            char * sec;
            char * family;
            char * qualifier;

            sec = ConfSectionListGetAt(seclist, i);

            if (ConfSectionParse(sec, &family, &qualifier) == 2) {
                snprintf(conf->errmsg, CLOG_ERRMSG_LEN_MAX, "found section: [%s:%s]", family, qualifier);

                if (!cstr_compare_len("rollingpolicy", (int)strlen("rollingpolicy"), family, -1, 0) && !strcmp(qualifier, rollingpolicy->str)) {
                    loaderror = 0;
                    snprintf(conf->errmsg, CLOG_ERRMSG_LEN_MAX, "success");

                    ncb = ConfReadValueParsed(cfgfile, family, qualifier, "rollingtime", readbuf, sizeof(readbuf));
                    if ( ncb-- > 1 ) {
                        char *rotstr = cstr_trim_whitespace(readbuf);
                        rollingtime_from_string(rotstr, cstr_length(rotstr, ncb), &conf->rollingtime);
                    }

                    ncb = ConfReadValueParsed(cfgfile, family, qualifier, "maxfilesize", readbuf, sizeof(readbuf));
                    if ( ncb ) {
                        conf->maxfilesize = (ub8) ConfParseSizeBytesValue(readbuf, (double) conf->maxfilesize, 0, 0);
                    }

                    ncb = ConfReadValueParsed(cfgfile, family, qualifier, "maxfilecount", readbuf, sizeof(readbuf));
                    if ( ncb ) {
                        conf->maxfilecount = (ub4) strtoul(readbuf, 0, 10);
                    }

                    ncb = ConfReadValueParsed(cfgfile, family, qualifier, "rollingappend", readbuf, sizeof(readbuf));
                    if ( ncb ) {
                        if (ConfParseBoolValue(readbuf, 1)) {
                            conf->rollingappend = 1;
                        }
                    }

                    goto found_policy;
                }
            }
        }
found_policy:
        ConfSectionListFree(seclist);
    }

    cstrbufFree(&rollingpolicy);
    return loaderror;
}
