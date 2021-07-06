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
 * @filename   clogger.c
 *  thread-safe c application logger.
 *
 * @author     Liang Zhang <350137278@qq.com>
 * @version    0.0.1
 * @create     2019-12-11 10:34:05
 * @update     2020-12-08 18:53:05
 */
#include <clogger_api.h>
#include <loggermgr_i.h>

static const char THIS_FILE[] = "clogger.c";

#if CLOG_MSGBUF_SIZE_MAX < CLOG_MSGBUF_SIZE_DEFAULT
#   undef CLOG_MSGBUF_SIZE_MAX
#   define CLOG_MSGBUF_SIZE_MAX  CLOG_MSGBUF_SIZE_DEFAULT
#endif


static const char clog_endcolor[] = "\033[0m";

static const char *clog_week_strs[]  = { 0, "Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun" };

static const char *clog_month_strs[]  = { 0, "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec" };

/* refer to: clog_level_t */
static const char *clog_level_strs[] = {"OFF", 0, 0, 0, "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE", "ALL", 0};
static const int   clog_level_lens[] = {    3, 0, 0, 0,       5,       5,      4,      4,       5,       5,    3,  0};


#if defined(__WINDOWS__)
    // same as: <unistd.h>
    # include <io.h>
    # include <process.h>

    # undef _USE_32BIT_TIME_T
    # include <sys/stat.h>

    # define stat64buf_t  struct __stat64
    # define lstat64      _stat64
#endif


typedef struct
{
    size_t fmtlen;
    char fmtbuf[CLOG_DATEFMT_SIZE_MAX];
} dateformat_buf;


typedef struct
{
    cstrbuf ident;
    clog_level_t level;

    size_t fmtlen;
    dateformat_buf dateminfmt, datetimefmt, stampidfmt;

    size_t startclrlen;
    char startclrfmt[32];

    int linenofmtlen;
    char linenofmt[160];

    int autowrapline;

#ifndef CLOGGER_NO_THREADNO
    int threadnofmtlen;
    char threadnofmt[32];
#endif

    size_t msglen;
    char *message;
} clog_message_fmt;


typedef struct
{
    size_t offsetcb;
    size_t dateminfmtlen;
    char dateminfmt[ROF_DATEMINUTE_SIZE];
    char message[0];
} clog_message_hdr;


static size_t clog_message_fmt_chunksize (const clog_message_fmt *msg, size_t maxmsgsize)
{
    size_t chunksize = ROF_DATEMINUTE_SIZE +
                cstrbufGetLen(msg->ident) +
                clog_level_lens[msg->level] +
                msg->fmtlen +
                msg->startclrlen +
                msg->linenofmtlen +
#ifndef CLOGGER_NO_THREADNO
                msg->threadnofmtlen +
#endif
                msg->msglen +
                32;

    chunksize = memapi_align_psize(chunksize);

    if (chunksize >= maxmsgsize) {
        return (-1);
    }

    return chunksize;
}


typedef struct _clog_logger_t
{
    struct {
        unsigned loctime        :1;
        unsigned timeunitms     :1;
        unsigned timeunitus     :1;
        unsigned timestampid    :1;

        unsigned autowrapline   :1;
        unsigned hideident      :1;
        unsigned rollingsize    :1;
        unsigned rollingtime    :1;

        unsigned appenderstdout :1;
        unsigned appendersyslog :1;
        unsigned appenderrofile :1;
        unsigned appendershmlog :1;

        unsigned levelcolors    :1;
        unsigned levelstyles    :1;
        unsigned filelineno     :1;
        unsigned function       :1;

#ifndef CLOGGER_NO_THREADNO
        unsigned processid      :1;
        unsigned threadno       :1;
#endif
    } bf;

    /* logger thread log messages */
    pthread_t logthread;
    pthread_mutex_t shutdownlock;

    /* logged messages counter */
    uatomic_int64 logmessages;
    uatomic_int64 logrounds;

    /* semaphore for ringbuffer */
    unsema_t sema;

    /* MT-safety ring buffer for queued logging messages */
    ring_buffer_st *ringbuffer;

    /* a memory pool for formating message */
    ringbuf_t *mempool;

    /* configuration */
    clog_level_t level;

    /* see: clog_level_t */
    clog_style_t levelstyles[12];
    clog_color_t levelcolors[12];

    clog_layout_t layout;

    clog_dateformat_t dateformat;

    /* rolling logging file */
    rollingfile_t logfile;

    /* shared memory map  for logging */
    shmmaplog_hdl shmlog;

    /* ident or category for loging */
    cstrbuf ident;

    /* unique id for logger */
    int loggerid;

    /* readonly max size for message */
    int maxmsgsize;

    /* global shared real time clock */
    rtclock_handle rtc;

    /* const string for process id */
    int pidcstrlen;
    char pidcstr[20];
} clog_logger_t;


/**
 * private api
 */
static rollingfile_t * clog_logger_get_rollingfile(clog_logger logger)
{
    return &logger->logfile;
}


static cstrbuf clog_replace_string (const char *source, int pairs, ...)
{
    cstrbuf sb = cstrbufNew(0, source, -1);

    va_list ap;
    va_start(ap, pairs);

    while (pairs-- > 0) {
        int rc;
        const char * replacee = va_arg(ap, const char *);
        const char * replacer = va_arg(ap, const char *);

        while ((rc = regex_match(replacee, sb->str)) != -1) {
            cstrbuf res = cstrbufSub(sb->str, rc, (int)strlen(replacee), replacer, (int)strlen(replacer));
            cstrbufFree(&sb);
            sb = res;
        }
    }

    va_end(ap);

    return sb;
}


static size_t clog_format_datetime (clog_logger logger, dateformat_buf *dateminfmt, dateformat_buf *datetimefmt, dateformat_buf *stampidfmt)
{
    size_t totalfmtlen = 0;
    struct timespec now = {0};
    struct tm loc = {0};
    int timezone = 0;
    int datlight = 0;
    const char *timezonefmt = TIMEZONE_FORMAT_UTC;
    if (logger->bf.loctime) {
        timezone = rtclock_timezone(logger->rtc, &timezonefmt);
        datlight = rtclock_daylight(logger->rtc);
    }
    rtclock_localtime(logger->rtc, timezone, datlight, &loc, &now);

    /* dateminfmt must be: "20191223-1157" */
    switch(logger->logfile.timepolicy) {
    case ROLLING_TM_MIN_1:
        dateminfmt->fmtlen = snprintf(dateminfmt->fmtbuf, sizeof(dateminfmt->fmtbuf), "%04d%02d%02d-%02d%02d",
                                loc.tm_year, loc.tm_mon, loc.tm_mday, loc.tm_hour, loc.tm_min);
        break;
    case ROLLING_TM_MIN_5:
        dateminfmt->fmtlen = snprintf(dateminfmt->fmtbuf, sizeof(dateminfmt->fmtbuf), "%04d%02d%02d-%02d%02d",
                                loc.tm_year, loc.tm_mon, loc.tm_mday, loc.tm_hour, (loc.tm_min/5) * 5);
        break;
    case ROLLING_TM_MIN_10:
        dateminfmt->fmtlen = snprintf(dateminfmt->fmtbuf, sizeof(dateminfmt->fmtbuf), "%04d%02d%02d-%02d%02d",
                                loc.tm_year, loc.tm_mon, loc.tm_mday, loc.tm_hour, (loc.tm_min/10) * 10);
        break;
    case ROLLING_TM_MIN_30:
        dateminfmt->fmtlen = snprintf(dateminfmt->fmtbuf, sizeof(dateminfmt->fmtbuf), "%04d%02d%02d-%02d%02d",
                                loc.tm_year, loc.tm_mon, loc.tm_mday, loc.tm_hour, (loc.tm_min/30) * 30);
        break;
    case ROLLING_TM_HOUR:
        dateminfmt->fmtlen = snprintf(dateminfmt->fmtbuf, sizeof(dateminfmt->fmtbuf), "%04d%02d%02d-%02d",
                                loc.tm_year, loc.tm_mon, loc.tm_mday, loc.tm_hour);
        break;
    case ROLLING_TM_DAY:
        dateminfmt->fmtlen = snprintf(dateminfmt->fmtbuf, sizeof(dateminfmt->fmtbuf), "%04d%02d%02d",
                                loc.tm_year, loc.tm_mon, loc.tm_mday);
        break;
    case ROLLING_TM_MON:
        dateminfmt->fmtlen = snprintf(dateminfmt->fmtbuf, sizeof(dateminfmt->fmtbuf), "%04d%02d",
                                loc.tm_year, loc.tm_mon);
        break;
    case ROLLING_TM_YEAR:
        dateminfmt->fmtlen = snprintf(dateminfmt->fmtbuf, sizeof(dateminfmt->fmtbuf), "%04d",
                                loc.tm_year);
        break;
    default:
        dateminfmt->fmtlen = 0;
        dateminfmt->fmtbuf[0] = 0;
        break;
    }
    totalfmtlen += dateminfmt->fmtlen;

    if (datetimefmt) {
        if (logger->dateformat == CLOG_DATEFMT_RFC_3339 || logger->dateformat == CLOG_DATEFMT_ISO_8601) {
            /* "2019-12-26 10:13:41+08:00", "2019-12-26T10:14:32+08:00" */
            char T = (logger->dateformat == CLOG_DATEFMT_ISO_8601?  'T' : 32);

            if (logger->bf.timeunitms) {
                datetimefmt->fmtlen = snprintf(datetimefmt->fmtbuf, sizeof(datetimefmt->fmtbuf),
                        "%04d-%02d-%02d%c%02d:%02d:%02d.%03d%.*s:%.*s", loc.tm_year, loc.tm_mon, loc.tm_mday, T, loc.tm_hour, loc.tm_min, loc.tm_sec, (int)(now.tv_nsec / 1000000), 3, timezonefmt, 2, timezonefmt + 3);
            } else if (logger->bf.timeunitus) {
                datetimefmt->fmtlen = snprintf(datetimefmt->fmtbuf, sizeof(datetimefmt->fmtbuf),
                        "%04d-%02d-%02d%c%02d:%02d:%02d.%06d%.*s:%.*s", loc.tm_year, loc.tm_mon, loc.tm_mday, T, loc.tm_hour, loc.tm_min, loc.tm_sec, (int)(now.tv_nsec / 1000), 3, timezonefmt, 2, timezonefmt + 3);
            } else {
                datetimefmt->fmtlen = snprintf(datetimefmt->fmtbuf, sizeof(datetimefmt->fmtbuf),
                        "%04d-%02d-%02d%c%02d:%02d:%02d%.*s:%.*s",      loc.tm_year, loc.tm_mon, loc.tm_mday, T, loc.tm_hour, loc.tm_min, loc.tm_sec, 3, timezonefmt, 2, timezonefmt + 3);
            }
        } else if (logger->dateformat == CLOG_DATEFMT_UNIVERSAL) {
            /* "Thu Dec 26 02:16:02 UTC 2019" */
            if (logger->bf.timeunitms) {
                datetimefmt->fmtlen = snprintf(datetimefmt->fmtbuf, sizeof(datetimefmt->fmtbuf),
                    "%.3s %.3s %02d %02d:%02d:%02d.%03d UTC%.*s %04d", clog_week_strs[loc.tm_wday], clog_month_strs[loc.tm_mon], loc.tm_mday, loc.tm_hour, loc.tm_min, loc.tm_sec, (int)(now.tv_nsec / 1000000), 5*logger->bf.loctime, timezonefmt, loc.tm_year);
            } else if (logger->bf.timeunitus) {
                datetimefmt->fmtlen = snprintf(datetimefmt->fmtbuf, sizeof(datetimefmt->fmtbuf),
                    "%.3s %.3s %02d %02d:%02d:%02d.%06d UTC%.*s %04d", clog_week_strs[loc.tm_wday], clog_month_strs[loc.tm_mon], loc.tm_mday, loc.tm_hour, loc.tm_min, loc.tm_sec, (int)(now.tv_nsec / 1000), 5*logger->bf.loctime, timezonefmt, loc.tm_year);
            } else {
                datetimefmt->fmtlen = snprintf(datetimefmt->fmtbuf, sizeof(datetimefmt->fmtbuf),
                    "%.3s %.3s %02d %02d:%02d:%02d UTC%.*s %04d", clog_week_strs[loc.tm_wday], clog_month_strs[loc.tm_mon], loc.tm_mday, loc.tm_hour, loc.tm_min, loc.tm_sec, 5*logger->bf.loctime, timezonefmt, loc.tm_year);
            }
        } else if (logger->dateformat == CLOG_DATEFMT_RFC_2822) {
            /* "Thu, 26 Dec 2019 10:12:45 +0800" */
            if (logger->bf.timeunitms) {
                datetimefmt->fmtlen = snprintf(datetimefmt->fmtbuf, sizeof(datetimefmt->fmtbuf),
                    "%.3s, %02d %.3s %04d %02d:%02d:%02d.%03d %.*s", clog_week_strs[loc.tm_wday], loc.tm_mday, clog_month_strs[loc.tm_mon], loc.tm_year, loc.tm_hour, loc.tm_min, loc.tm_sec, (int)(now.tv_nsec / 1000000), 5, timezonefmt);
            } else if (logger->bf.timeunitus) {
                datetimefmt->fmtlen = snprintf(datetimefmt->fmtbuf, sizeof(datetimefmt->fmtbuf),
                    "%.3s, %02d %.3s %04d %02d:%02d:%02d.%06d %.*s", clog_week_strs[loc.tm_wday], loc.tm_mday, clog_month_strs[loc.tm_mon], loc.tm_year, loc.tm_hour, loc.tm_min, loc.tm_sec, (int)(now.tv_nsec / 1000), 5, timezonefmt);
            } else {
                datetimefmt->fmtlen = snprintf(datetimefmt->fmtbuf, sizeof(datetimefmt->fmtbuf),
                    "%.3s, %02d %.3s %04d %02d:%02d:%02d %.*s", clog_week_strs[loc.tm_wday], loc.tm_mday, clog_month_strs[loc.tm_mon], loc.tm_year, loc.tm_hour, loc.tm_min, loc.tm_sec, 5, timezonefmt);
            }
        } else {
            /* CLOG_DATEFMT_NUMERIC_1 = "20191226101245+0800", CLOG_DATEFMT_NUMERIC_2 = "20191226-101245+0800" */
            char minuschr[2] = {'-', '\0'};

            if (logger->dateformat == CLOG_DATEFMT_NUMERIC_1) {
                minuschr[0] = '\0';
            }

            if (logger->bf.timeunitms) {
                datetimefmt->fmtlen = snprintf(datetimefmt->fmtbuf, sizeof(datetimefmt->fmtbuf),
                    "%04d%02d%02d%s%02d%02d%02d.%03d%.*s", loc.tm_year, loc.tm_mon, loc.tm_mday, minuschr, loc.tm_hour, loc.tm_min, loc.tm_sec, (int)(now.tv_nsec / 1000000), 5, timezonefmt);
            } else if (logger->bf.timeunitus) {
                datetimefmt->fmtlen = snprintf(datetimefmt->fmtbuf, sizeof(datetimefmt->fmtbuf),
                    "%04d%02d%02d%s%02d%02d%02d.%06d%.*s", loc.tm_year, loc.tm_mon, loc.tm_mday, minuschr, loc.tm_hour, loc.tm_min, loc.tm_sec, (int)(now.tv_nsec / 1000), 5, timezonefmt);
            } else {
                datetimefmt->fmtlen = snprintf(datetimefmt->fmtbuf, sizeof(datetimefmt->fmtbuf),
                    "%04d%02d%02d%s%02d%02d%02d%.*s", loc.tm_year, loc.tm_mon, loc.tm_mday, minuschr, loc.tm_hour, loc.tm_min, loc.tm_sec, 5, timezonefmt);
            }
        }

        totalfmtlen += datetimefmt->fmtlen;
    }

    if (stampidfmt) {
        if (logger->bf.timestampid) {
            stampidfmt->fmtlen = logger_manager_get_stampid(stampidfmt->fmtbuf, (int)sizeof(stampidfmt->fmtbuf));
        } else {
            stampidfmt->fmtlen = 0;
            stampidfmt->fmtbuf[0] = 0;
        }
        totalfmtlen += stampidfmt->fmtlen;
    }

    /* all success */
    return totalfmtlen;
}


static void write_message_cb (char *chunkbuf, size_t chunkbufsz, void *entry)
{
    const clog_message_fmt *msg = (const clog_message_fmt *) entry;

    clog_message_hdr *msghdr = (clog_message_hdr *) chunkbuf;

    char *msgbuf = msghdr->message;
    size_t msgcb = 0;

    msghdr->dateminfmtlen = msg->dateminfmt.fmtlen;
    memcpy(msghdr->dateminfmt, msg->dateminfmt.fmtbuf, msghdr->dateminfmtlen);

    if (msg->stampidfmt.fmtlen) {
        memcpy(msgbuf + msgcb, msg->stampidfmt.fmtbuf, msg->stampidfmt.fmtlen);
        msgcb += msg->stampidfmt.fmtlen;
        msgbuf[msgcb++] = 32;
    }

    if (msg->startclrlen) {
        memcpy(msgbuf + msgcb, msg->startclrfmt, msg->startclrlen);
        msgcb += msg->startclrlen;
    }

    if (msg->datetimefmt.fmtlen) {
        memcpy(msgbuf + msgcb, msg->datetimefmt.fmtbuf, msg->datetimefmt.fmtlen);
        msgcb += msg->datetimefmt.fmtlen;
        msgbuf[msgcb++] = 32;
    }

    memcpy(msgbuf + msgcb, clog_level_strs[msg->level], clog_level_lens[msg->level]);
    msgcb += clog_level_lens[msg->level];
    msgbuf[msgcb++] = 32;

    if (msg->ident) {
        msgbuf[msgcb++] = '<';
        memcpy(msgbuf + msgcb, msg->ident->str, msg->ident->len);
        msgcb += msg->ident->len;
        msgbuf[msgcb++] = '>';
        msgbuf[msgcb++] = 32;
    }

    if (msg->linenofmtlen) {
        memcpy(msgbuf + msgcb, msg->linenofmt, msg->linenofmtlen);
        msgcb += msg->linenofmtlen;
        msgbuf[msgcb++] = 32;
    }

#ifndef CLOGGER_NO_THREADNO
    if (msg->threadnofmtlen) {
        memcpy(msgbuf + msgcb, msg->threadnofmt, msg->threadnofmtlen);
        msgcb += msg->threadnofmtlen;
        msgbuf[msgcb++] = 32;
    }
#endif

    if (msg->startclrlen) {
        memcpy(msgbuf + msgcb, clog_endcolor, 4);
        msgcb += 4;
    }

    memcpy(msgbuf + msgcb, msg->message, msg->msglen);
    msgcb += msg->msglen;

    if (msg->autowrapline) {
        if (msgbuf[msgcb - 1] != '\n') {
            msgbuf[msgcb++] = '\n';
        }
    }

    msghdr->offsetcb = sizeof(*msghdr) + msgcb;
}


static int read_message_cb (const ringbuf_entry_st *entry, void *arg)
{
    clog_logger logger = (clog_logger) arg;

    int wok;

    clog_message_hdr *msghdr = (clog_message_hdr *) entry->chunk;
    size_t messagelen = msghdr->offsetcb - sizeof(*msghdr);

    if (logger->bf.appenderstdout) {
        fprintf(stdout, "%.*s", (int) messagelen, msghdr->message);
    }

    if (logger->bf.appendersyslog) {
        int priority = (LOG_DEBUG + 1);

        switch (logger->level) {
        case CLOG_LEVEL_FATAL:
            priority = LOG_EMERG;
            break;

        case CLOG_LEVEL_ERROR:
            priority = LOG_ERR;
            break;

        case CLOG_LEVEL_WARN:
            priority = LOG_WARNING;
            break;

        case CLOG_LEVEL_INFO:
            priority = LOG_INFO;
            break;

        case CLOG_LEVEL_DEBUG:
            priority = LOG_DEBUG;
            break;

        case CLOG_LEVEL_ALL:
        case CLOG_LEVEL_TRACE:
        case CLOG_LEVEL_OFF:
            break;
        }

        if (priority <= LOG_DEBUG) {
            syslog(LOG_USER | priority, "%.*s", (int)messagelen, msghdr->message);
        }
    }

    wok = 0;
    if (logger->bf.appendershmlog) {
        wok = shmmaplog_write(logger->shmlog, msghdr->message, messagelen);
    }

    if (!wok && logger->bf.appenderrofile) {
        rollingfile_write(&logger->logfile, msghdr->dateminfmt, (int)msghdr->dateminfmtlen, msghdr->message, messagelen);
    }

    if (uatomic_int64_add(&logger->logmessages) == SB8MAXVAL) {
        uatomic_int64_zero(&logger->logmessages);
        uatomic_int64_add(&logger->logrounds);
    }

    return 1;
}


static void * clog_threadfunc (void *arg)
{
    clog_logger logger = (clog_logger) arg;

    while (pthread_mutex_trylock(&logger->shutdownlock) != 0) {
        if (unsema_timedwait(&logger->sema, 1000) == 0) {
            ringbufst_read_next(logger->ringbuffer, read_message_cb, logger);
        }
    }

    pthread_mutex_destroy(&logger->shutdownlock);
    return (void*) 0;
}


/**
 * public helper api
 */
void clog_set_levelcolor(clog_logger logger, clog_level_t level, clog_color_t color)
{
    logger->levelcolors[ level ] = color;
}


void clog_set_levelstyle(clog_logger logger, clog_level_t level, clog_style_t style)
{
    logger->levelstyles[ level ] = style;
}


int clog_level_from_string (const char *levelstring, int length, clog_level_t *level)
{
    int lvl = cstr_findstr_in(levelstring, length, clog_level_strs, (int)(sizeof(clog_level_strs)/sizeof(clog_level_strs[0])), 1);

    if (lvl != -1) {
        *level = (clog_level_t) lvl;

        /* success */
        return 1;
    }

    /* failed as default */
    return 0;
}


int clog_layout_from_string(const char *layoutstring, int length, clog_layout_t *layout)
{
    if (!cstr_compare_len(layoutstring, length, "PLAIN", 5, 1)) {
        *layout = CLOG_LAYOUT_PLAIN;
        return 1;
    }

    if (!cstr_compare_len(layoutstring, length, "DATED", 5, 1)) {
        *layout = CLOG_LAYOUT_DATED;
        return 1;
    }

    /* failed as default */
    return 0;
}


int clog_dateformat_from_string(const char *datefmtstring, int length, clog_dateformat_t *dateformat)
{
    if (!cstr_compare_len(datefmtstring, length, "UTC", 3, 1)) {
        *dateformat = CLOG_DATEFMT_UNIVERSAL;
        return 1;
    }
    if (!cstr_compare_len(datefmtstring, length, "UNIVERSAL", 9, 1)) {
        *dateformat = CLOG_DATEFMT_UNIVERSAL;
        return 1;
    }

    if (!cstr_compare_len(datefmtstring, length, "NUMERIC-1", 9, 1)) {
        *dateformat = CLOG_DATEFMT_NUMERIC_1;
        return 1;
    }
    if (!cstr_compare_len(datefmtstring, length, "NUMERIC", 7, 1)) {
        *dateformat = CLOG_DATEFMT_NUMERIC_1;
        return 1;
    }

    if (!cstr_compare_len(datefmtstring, length, "NUMERIC-2", 9, 1)) {
        *dateformat = CLOG_DATEFMT_NUMERIC_2;
        return 1;
    }

    if (!cstr_compare_len(datefmtstring, length, "RFC-3339", 8, 1)) {
        *dateformat = CLOG_DATEFMT_RFC_3339;
        return 1;
    }
    if (!cstr_compare_len(datefmtstring, length, "DEFAULT", 7, 1)) {
        *dateformat = CLOG_DATEFMT_RFC_3339;
        return 1;
    }

    if (!cstr_compare_len(datefmtstring, length, "ISO-8601", 8, 1)) {
        *dateformat = CLOG_DATEFMT_ISO_8601;
        return 1;
    }

    if (!cstr_compare_len(datefmtstring, length, "RFC-2822", 8, 1)) {
        *dateformat = CLOG_DATEFMT_RFC_2822;
        return 1;
    }

    /* failed as default */
    return 0;
}


int clog_appender_from_string (const char *appenderstring, int length, int *appender)
{
    int appenders = 0;

    cstrbuf apstr = cstrbufNew(0, appenderstring, length);
    cstr_toupper(apstr->str, apstr->len);

    if (cstr_containwith(apstr->str, apstr->len, "STDOUT", 6) != -1) {
        appenders |= CLOG_APPENDER_STDOUT;
    }
    if (cstr_containwith(apstr->str, apstr->len, "ROFILE", 6) != -1) {
        appenders |= CLOG_APPENDER_ROFILE;
    }
    if (cstr_containwith(apstr->str, apstr->len, "SYSLOG", 6) != -1) {
        appenders |= CLOG_APPENDER_SYSLOG;
    }
    if (cstr_containwith(apstr->str, apstr->len, "SHMLOG", 6) != -1) {
        appenders |= CLOG_APPENDER_SHMMAP;
    }

    if (!appenders) {
        /* failed as default */
        cstrbufFree(&apstr);
        return 0;
    }

    *appender = appenders;
    cstrbufFree(&apstr);
    return 1;
}


/**
 * public api
 */
clog_logger clog_logger_create (clogger_conf conf, logger_manager mgr)
{
    clog_logger_t *logger;

    struct timespec ts;

    char timestr[22] = {0};

    cstrbuf namepatternRep = 0;
    cstrbuf pathprefixRep = 0;
    cstrbuf shmlogfileRep = 0;

    ub4 flags = (int) logger_conf_get_creatflags(conf);

    CHKCONFIG_INT_VALUE(CLOG_MSGBUF_SIZE_DEFAULT, CLOG_MSGBUF_SIZE_MIN, CLOG_MSGBUF_SIZE_MAX, conf->maxmsgsize);

    CHKCONFIG_INT_VALUE(128, 64, 1024, conf->maxconcurrents);
    if (conf->maxconcurrents > conf->queuelength) {
        conf->maxconcurrents = memapi_align_psize(conf->queuelength);
    }

    getnowtimeofday(&ts);
    snprintf(timestr, sizeof(timestr), "%"PRId64, (int64_t)ts.tv_sec);

    logger = (clog_logger_t *) mem_alloc_zero(1, sizeof(*logger));

    logger->rtc = mgr->rtclock;

    logger->pidcstrlen = snprintf(logger->pidcstr, sizeof(logger->pidcstr), "%d", getprocessid());

    if (CLOG_TIMEUNIT_MSEC & flags) {
        logger->bf.timeunitms = 1;
        logger->bf.timeunitus = 0;
    }
    if (CLOG_TIMEUNIT_USEC & flags) {
        logger->bf.timeunitms = 0;
        logger->bf.timeunitus = 1;
    }
    if (CLOG_TIMESTAMP_ID & flags) {
        logger->bf.timestampid = 1;
    }

    if (CLOG_TIMEZONE_LOC & flags) {
        logger->bf.loctime = 1;
    }

    if (CLOG_ROLLING_SIZE_BASED & flags) {
        logger->bf.rollingsize = 1;
    }
    if (CLOG_ROLLING_TIME_BASED & flags) {
        logger->bf.rollingtime = 1;
    }

    if (CLOG_APPENDER_STDOUT & flags) {
        logger->bf.appenderstdout = 1;
    }
    if (CLOG_APPENDER_SYSLOG & flags) {
        logger->bf.appendersyslog = 1;
    }
    if (CLOG_APPENDER_ROFILE & flags) {
        logger->bf.appenderrofile = 1;
    }
    if (CLOG_APPENDER_SHMMAP & flags) {
        logger->bf.appendershmlog = 1;
    }

    if (CLOG_LEVEL_COLORS & flags) {
        logger->bf.levelcolors = 1;
    }
    if (CLOG_LEVEL_STYLES & flags) {
        logger->bf.levelstyles = 1;
    }

    if (CLOG_FILE_LINENO & flags) {
        logger->bf.filelineno = 1;
    }
    if (CLOG_FUNCTION_NAME & flags) {
        logger->bf.function = 1;
    }

    if (conf->autowrapline) {
        logger->bf.autowrapline = 1;
    }

    if (conf->hideident) {
        logger->bf.hideident = 1;
    }

#ifndef CLOGGER_NO_THREADNO
    if (conf->processid) {
        logger->bf.processid = 1;
    }

    if (conf->threadno) {
        logger->bf.threadno = 1;
    }
#endif

    logger->maxmsgsize = conf->maxmsgsize;
    logger->ident = cstrbufDup(0, conf->ident->str, conf->ident->len);

    logger->mempool = ringbuf_init(conf->maxconcurrents);
    while (conf->maxconcurrents-- > 0) {
        ringbuf_elt_t *eltp;
        ringbuf_elt_new(conf->maxmsgsize, mem_free, &eltp);
        if (ringbuf_push(logger->mempool, eltp) != 1) {
            ringbuf_elt_free(eltp);
        }
    }

    logger->ringbuffer = ringbufst_init(conf->queuelength, conf->maxmsgsize);

    if (logger->bf.appenderrofile) {
        namepatternRep = clog_replace_string(cstrbufGetStr(conf->nameprefix), 3, "<IDENT>", cstrbufGetStr(logger->ident), "<PID>", logger->pidcstr, "<DATE>", timestr);
        pathprefixRep = clog_replace_string(cstrbufGetStr(conf->pathprefix),   3, "<IDENT>", cstrbufGetStr(logger->ident), "<PID>", logger->pidcstr, "<DATE>", timestr);

        rollingfile_init(&logger->logfile, cstrbufGetStr(pathprefixRep), cstrbufGetStr(namepatternRep));
    }

    if (logger->bf.appendershmlog) {
        int err;
        md5sum_t ctx;

        shmlogfileRep = clog_replace_string(cstrbufGetStr(conf->shmlogfile), 3, "<IDENT>", cstrbufGetStr(logger->ident), "<PID>", logger->pidcstr, "<DATE>", timestr);

        md5sum_init(&ctx, conf->magickey);
        md5sum_updt(&ctx, (const uint8_t *) shmlogfileRep->str, shmlogfileRep->len);
        md5sum_done(&ctx, ctx.digest);

        err = shmmaplog_init(cstrbufGetStr(pathprefixRep), cstrbufGetStr(shmlogfileRep), (conf->maxmsgsize * conf->queuelength), ctx.digest, &logger->shmlog);
        if (err) {
            mem_free(logger);
            emerglog_exit("libclogger", "shmmaplog_init error(%d)", err);
        }
    }

    cstrbufFree(&namepatternRep);
    cstrbufFree(&pathprefixRep);
    cstrbufFree(&shmlogfileRep);

    if (unsema_init(&logger->sema, 0) != 0) {
        emerglog_exit("libclogger", "unsema_init error(%d)", errno);
    }

    if (pthread_mutex_init(&logger->shutdownlock, NULL) == -1) {
        emerglog_exit("libclogger", "pthread_mutex_init failed");
    }

    if (pthread_mutex_lock(&logger->shutdownlock) != 0) {
        emerglog_exit("libclogger", "pthread_mutex_lock failed");
    }

    /* set default colors and styles */
    clog_set_levelcolor(logger, CLOG_LEVEL_FATAL, CLOG_COLOR_RED);
    clog_set_levelcolor(logger, CLOG_LEVEL_ERROR, CLOG_COLOR_PURPLE);
    clog_set_levelcolor(logger, CLOG_LEVEL_WARN,  CLOG_COLOR_YELLOW);
    clog_set_levelcolor(logger, CLOG_LEVEL_INFO,  CLOG_COLOR_CYAN);
    clog_set_levelcolor(logger, CLOG_LEVEL_DEBUG, CLOG_COLOR_GREEN);
    clog_set_levelcolor(logger, CLOG_LEVEL_TRACE, CLOG_COLOR_NOCLR);

    clog_set_levelstyle(logger, CLOG_LEVEL_FATAL, CLOG_STYLE_LIGHT);
    clog_set_levelstyle(logger, CLOG_LEVEL_ERROR, CLOG_STYLE_LIGHT);
    clog_set_levelstyle(logger, CLOG_LEVEL_WARN,  CLOG_STYLE_LIGHT);
    clog_set_levelstyle(logger, CLOG_LEVEL_INFO,  CLOG_STYLE_NORMAL);
    clog_set_levelstyle(logger, CLOG_LEVEL_DEBUG, CLOG_STYLE_NORMAL);
    clog_set_levelstyle(logger, CLOG_LEVEL_TRACE, CLOG_STYLE_NORMAL);

    rollingfile_t * rof = &(logger->logfile);

    rollingfile_set_timepolicy(rof, conf->rollingtime);
    rollingfile_set_sizepolicy(rof, conf->maxfilesize, conf->maxfilecount, conf->rollingappend);

    /* copy config for logger */
    logger->level = conf->loglevel;
    logger->layout = conf->layout;
    logger->dateformat = conf->dateformat;

    if (logger->bf.appendersyslog) {
#if defined(__WINDOWS__)
        if (conf->winsyslogconf) {
            /* default: "localhost:514" */
            set_syslog_conf_dir(cstrbufGetStr(conf->winsyslogconf));
        }
#endif
        openlog(logger->ident->str, LOG_PID | LOG_NDELAY | LOG_NOWAIT, 0);
    }

    /* success */
    logger->loggerid = conf->loggerid;
    logger_conf_final_release(conf);

    /* start running logthread */
    if (pthread_create(&logger->logthread, NULL, clog_threadfunc, (void*)logger) == -1) {
        emerglog_exit("libclogger", "pthread_create failed");
    }

    return logger;
}


void clog_logger_destroy(clog_logger logger)
{
    pthread_mutex_unlock(&logger->shutdownlock);

    unsema_post(&logger->sema);

    pthread_join(logger->logthread, NULL);

    unsema_uninit(&logger->sema);

    cstrbufFree(&logger->ident);

    rollingfile_uninit(&logger->logfile);

    shmmaplog_uninit(logger->shmlog);

    if (logger->bf.appendersyslog) {
        closelog();
    }

    ringbufst_uninit(logger->ringbuffer);

    ringbuf_uninit(logger->mempool);

    mem_free(logger);
}

int clog_logger_get_loggerid(clog_logger logger)
{
    return logger->loggerid;
}


const char * clog_logger_get_ident(clog_logger logger)
{
    return logger->ident->str;
}


int64_t clog_logger_get_logmessages(clog_logger logger, int64_t *round)
{
    if (round) {
        *round = logger->logrounds;
    }
    return uatomic_int64_get(&logger->logmessages);
}


int clog_logger_get_maxmsgsize(clog_logger logger)
{
    return logger->maxmsgsize;
}


int clog_logger_level_enabled(clog_logger logger, clog_level_t level)
{
    if (! logger) {
        /* invalid logger */
        return 0;
    }

    if (level == CLOG_LEVEL_OFF || level == CLOG_LEVEL_ALL) {
        /* invalid qurey level */
        return 0;
    }

    if (logger->level == CLOG_LEVEL_OFF) {
        /* disable all logger levels */
        return 0;
    }

    /* check log level */
    if (logger->level == CLOG_LEVEL_ALL || level <= logger->level) {
        return 1;
    }

    /* level is disabled */
    return 0;
}


const char * clog_logger_file_basename (const char *pathname, int *namelen)
{
    int pathlen = *namelen;
    int i = pathlen;
    while (i-- > 1) {
        if (pathname[i] == '/' || pathname[i] == '\\') {
            break;
        }
    }

    if (i == 0) {
        *namelen = pathlen;
        return pathname;
    } else {
        *namelen = pathlen - i - 1;
        return pathname + i + 1;
    }
}


static void logger_commit_message (clog_logger logger, const clog_message_fmt *msg, ub2 maxwaitms, int intervalms)
{
    int waitms = 0;

    size_t chunksize  = clog_message_fmt_chunksize(msg, logger->maxmsgsize);
    if (chunksize == -1) {
        /* message is oversize */
        return;
    }

    while (! ringbufst_write(logger->ringbuffer, chunksize, write_message_cb, (void*) msg)) {
        if (! maxwaitms) {
            /* failed push msg with nowait */
            break;
        }

        if (maxwaitms == CLOG_MSGWAIT_INFINITE) {
            /* wait forever until push msg ok */
            sleep_msec(intervalms);
            continue;
        }

        if (waitms < (int)maxwaitms) {
            int ms = (int)maxwaitms - waitms;
            if (ms < intervalms) {
                sleep_msec(ms);
                waitms += ms;
            } else {
                sleep_msec(intervalms);
                waitms += intervalms;
            }
            continue;
        }

        /* failed push msg with maxwaitms elapsed */
        break;
    }

    unsema_post(&logger->sema);
}


void clog_logger_log_message (clog_logger logger, clog_level_t level, uint16_t maxwaitms, const char *message, int msglen)
{
    if (level > logger->level) {
        /* logger not enabled for given level */
        return;
    }
    if (msglen < 0) {
        msglen = cstr_length(message, logger->maxmsgsize - 1);
    }
    if (! msglen) {
        return;
    }
    if (msglen >= (int)logger->maxmsgsize) {
        /* need to truncate message */
        msglen = (int)logger->maxmsgsize - 1;
    }

    if (logger->layout == CLOG_LAYOUT_PLAIN) {
        clog_message_fmt msgfmt;
        bzero(&msgfmt, sizeof(msgfmt));

        msgfmt.fmtlen = clog_format_datetime(logger, &msgfmt.dateminfmt, NULL, NULL);
        msgfmt.msglen = msglen;
        msgfmt.message = (char*) message;

        logger_commit_message(logger, &msgfmt, maxwaitms, (int)CLOG_MSGWAIT_INSTANT);
    } else if (logger->layout == CLOG_LAYOUT_DATED) {
        clog_message_fmt msgfmt;
        bzero(&msgfmt, sizeof(msgfmt));

        msgfmt.level = level;
        if (! logger->bf.hideident) {
            msgfmt.ident = logger->ident;
        }
        msgfmt.autowrapline = logger->bf.autowrapline;

        msgfmt.fmtlen = clog_format_datetime(logger, &msgfmt.dateminfmt, &msgfmt.datetimefmt, &msgfmt.stampidfmt);
        msgfmt.msglen = msglen;
        msgfmt.message = (char*) message;

        if (logger->bf.levelcolors) {
            clog_style_t style = CLOG_STYLE_NORMAL;
            clog_style_t color = logger->levelcolors[level];
            if (logger->bf.levelstyles) {
                style = logger->levelstyles[level];
            }
            msgfmt.startclrlen = snprintf(msgfmt.startclrfmt, sizeof(msgfmt.startclrfmt), "\033[%d;%dm", style, color);
        }

        logger_commit_message(logger, &msgfmt, maxwaitms, (int)CLOG_MSGWAIT_INSTANT);
    }
}


int clog_logger_get_timezone(clog_logger logger, const char **timezonefmt)
{
    return rtclock_timezone(logger->rtc, timezonefmt);
}


int clog_logger_get_daylight(clog_logger logger)
{
    return rtclock_daylight(logger->rtc);
}


int64_t clog_logger_get_ticktime(clog_logger logger, struct timespec *ticktime)
{
    return rtclock_ticktime(logger->rtc, ticktime);
}


void clog_logger_get_localtime(clog_logger logger, int timezone, int daylight, struct tm *tmloc, struct timespec *now)
{
    rtclock_localtime(logger->rtc, timezone, daylight, tmloc, now);
}


/**
 * format log message
 * Format:
 *   YYYYMMDD-hh:mm:ss.ddd GMT+H:mm $LEVEL $ident- ($filename:$lineno) <$function> [$processid/$threadno] Error message
 * Sample:
 *   20191222-17:35:28.188 GMT+8 INFO client- (main.c:69) <runforever> [1899/1] Cannot open file: invalid path - '/cygdrive/c/test/log'
 */
void clog_logger_log_format(clog_logger logger, clog_level_t level, uint16_t maxwaitms, const char *filename, int lineno, const char *funcname, const char *format, ...)
{
    int msglen = 0;

    if (level > logger->level) {
        /* logger not enabled for given level */
        return;
    }

    if (logger->layout == CLOG_LAYOUT_PLAIN) {
        clog_message_fmt msgfmt;
        ringbuf_elt_t *msgbuf;

        bzero(&msgfmt, sizeof(msgfmt));
        ringbuf_pop_always(logger->mempool, msgbuf);

        msgfmt.fmtlen = clog_format_datetime(logger, &msgfmt.dateminfmt, NULL, NULL);

        va_list args;
        va_start(args, format);
        msgfmt.msglen = vsnprintf(msgbuf->data, msgbuf->size, format, args);
        va_end(args);

        if (msgfmt.msglen == -1) {
            msgfmt.msglen = snprintf(msgbuf->data, msgbuf->size, "application error");
        } else if (msgfmt.msglen >= msgbuf->size) {
            /* message was truncated due to maxmsgsize limit */
            msgfmt.msglen = logger->maxmsgsize - 1;

            msgbuf->data[msgfmt.msglen - 3] = '.';
            msgbuf->data[msgfmt.msglen - 2] = '.';
            msgbuf->data[msgfmt.msglen - 1] = '.';
        }

        msgbuf->data[msgfmt.msglen] = '\0';
        msgfmt.message = msgbuf->data;

        logger_commit_message(logger, &msgfmt, maxwaitms, (int)CLOG_MSGWAIT_INSTANT);

        ringbuf_push_always(logger->mempool, msgbuf);
    } else if (logger->layout == CLOG_LAYOUT_DATED) {
        clog_message_fmt msgfmt;
        ringbuf_elt_t *msgbuf;

        bzero(&msgfmt, sizeof(msgfmt));
        ringbuf_pop_always(logger->mempool, msgbuf);

        msgfmt.level = level;
        if (! logger->bf.hideident) {
            msgfmt.ident = logger->ident;
        }
        msgfmt.autowrapline = logger->bf.autowrapline;

        msgfmt.fmtlen = clog_format_datetime(logger, &msgfmt.dateminfmt, &msgfmt.datetimefmt, &msgfmt.stampidfmt);

        if (logger->bf.levelcolors) {
            clog_style_t style = CLOG_STYLE_NORMAL;
            clog_style_t color = logger->levelcolors[level];
            if (logger->bf.levelstyles) {
                style = logger->levelstyles[level];
            }
            msgfmt.startclrlen = snprintf(msgfmt.startclrfmt, sizeof(msgfmt.startclrfmt), "\033[%d;%dm", style, color);
        }

        if (logger->bf.filelineno && filename) {
            int basenamelen = cstr_length(filename, 256);
            const char *basename = clog_logger_file_basename(filename, &basenamelen);
            if (basenamelen > 84) {
                basenamelen = 84;
            }

            if (logger->bf.function) {
                msgfmt.linenofmtlen = snprintf(msgfmt.linenofmt, sizeof(msgfmt.linenofmt), "(%.*s:%d::%.*s)", basenamelen, basename, lineno, cstr_length(funcname, 60), funcname);
            } else {
                msgfmt.linenofmtlen = snprintf(msgfmt.linenofmt, sizeof(msgfmt.linenofmt), "(%.*s:%d)", basenamelen, basename, lineno);
            }
        }

#ifndef CLOGGER_NO_THREADNO
        if (logger->bf.processid) {
            if (logger->bf.threadno) {
                msgfmt.threadnofmtlen = snprintf(msgfmt.threadnofmt, sizeof(msgfmt.threadnofmt), "[%.*s/%d]", logger->pidcstrlen, logger->pidcstr, (int)getthreadid());
            } else {
                msgfmt.threadnofmtlen = snprintf(msgfmt.threadnofmt, sizeof(msgfmt.threadnofmt), "[%.*s]", logger->pidcstrlen, logger->pidcstr);
            }
        }
#endif

        va_list args;
        va_start(args, format);
        msgfmt.msglen = vsnprintf(msgbuf->data, msgbuf->size, format, args);
        va_end(args);

        if (msgfmt.msglen == -1) {
            msgfmt.msglen = snprintf(msgbuf->data, msgbuf->size, "application error");
        } else if (msgfmt.msglen >= msgbuf->size) {
            /* message was truncated due to maxmsgsize limit */
            msgfmt.msglen = logger->maxmsgsize - 1;

            msgbuf->data[msgfmt.msglen - 3] = '.';
            msgbuf->data[msgfmt.msglen - 2] = '.';
            msgbuf->data[msgfmt.msglen - 1] = '.';
        }

        msgbuf->data[msgfmt.msglen] = '\0';
        msgfmt.message = msgbuf->data;

        logger_commit_message(logger, &msgfmt, maxwaitms, (int)CLOG_MSGWAIT_INSTANT);

        ringbuf_push_always(logger->mempool, msgbuf);
    }
}
