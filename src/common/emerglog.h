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
 * @filename   emerglog.h
 *  syslog for EMERG event message.
 *
 *  see syslog as below:
 *    $ cat /var/log/messages | grep $ident
 *
 * @author     Liang Zhang <350137278@qq.com>
 * @version    0.0.10
 * @create     2017-05-02 12:02:50
 * @update     2021-06-22 18:29:50
 */
#ifndef EMERG_LOG_H_INCLUDED
#define EMERG_LOG_H_INCLUDED

/**
 * close "Message from syslogd@code ..." to terminals on el6:
 *
 *  1) Comment out the line "*.emerg ... *" in file '/etc/rsyslog.conf' like below:
 *
 *  # Everybody gets emergency messages
 *  #!-- *.emerg         *
 *
 *  2) Then restart rsyslog service:
 *      $ /etc/init.d/rsyslog restart
 */


#if defined(__cplusplus)
extern "C"
{
#endif

#if defined (__WINDOWS__)
    #ifndef _INC_WINDOWS
    # include <WinSock2.h>
    #endif

    # include "win32/syslog.h"
#else
    # include <sys/time.h>
    # include <syslog.h>
#endif


#ifndef EMERGLOG_MSGLEN
#   define EMERGLOG_MSGLEN   1023
#endif

#ifndef EMERGLOG_IDENT
#   define EMERGLOG_IDENT    "emerglog.h"
#endif


#ifndef EMERGLOG_OPTION
#   define EMERGLOG_OPTION   (LOG_PID | LOG_NDELAY | LOG_NOWAIT | LOG_PERROR)
#endif


NOWARNING_UNUSED(static)
void emerg_syslog_message(int exitcode, const char *ident, const char *filename, int lineno, const char *format, ...)
{
    char msgbuf[EMERGLOG_MSGLEN + 1];
    int chlen = 0;

    va_list args;
    va_start(args, format);
    chlen = vsnprintf(msgbuf, sizeof(msgbuf), format, args);
    va_end(args);

    if (chlen > EMERGLOG_MSGLEN) {
        chlen = EMERGLOG_MSGLEN;
    } else if (chlen < 0) {
        chlen = 0;
    }

    msgbuf[chlen] = '\0';

    if (ident) {
        openlog(ident, EMERGLOG_OPTION, 0);
    } else {
        openlog(EMERGLOG_IDENT, EMERGLOG_OPTION, 0);
    }

    if (filename && lineno) {
        syslog(LOG_USER | LOG_EMERG, "(%s:%d) %.*s\n", filename, lineno, chlen, msgbuf);
    } else {
        syslog(LOG_USER | LOG_EMERG, "%.*s\n", chlen, msgbuf);
    }

    closelog();

    if (exitcode) {
        exit(exitcode);
    }
}


#define emerglog_oom_exit(ptr, ident)   do { \
        if (! (ptr)) { \
            emerg_syslog_message(EXIT_FAILURE, ident, __FILE__, __LINE__, "FATAL: no memory allocated"); \
        } \
    } while(0)


#if defined(_MSC_VER)

    # define emerglog_err_exit(err, ident, message, ...) do { \
        if ((err)) { \
            emerg_syslog_message(EXIT_FAILURE, ident, __FILE__, __LINE__, message, __VA_ARGS__); \
        } \
    } while(0)

    # define emerglog_exit(ident, message, ...) \
        emerg_syslog_message(EXIT_FAILURE, ident, __FILE__, __LINE__, message, __VA_ARGS__)

    # define emerglog_msg(ident, message, ...) \
        emerg_syslog_message(0, ident, __FILE__, __LINE__, message, __VA_ARGS__)

#else

    # define emerglog_err_exit(err, ident, message, args...) do { \
        if ((err)) { \
            emerg_syslog_message(EXIT_FAILURE, ident, __FILE__, __LINE__, message, ##args); \
        } \
    } while(0)

    # define emerglog_exit(ident, message, args...) \
        emerg_syslog_message(EXIT_FAILURE, ident, __FILE__, __LINE__, message, ##args)

    # define emerglog_msg(ident, message, args...) \
        emerg_syslog_message(0, ident, __FILE__, __LINE__, message, ##args)

#endif

#if defined(__cplusplus)
}
#endif

#endif /* EMERG_LOG_H_INCLUDED */
