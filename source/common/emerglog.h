/*******************************************************************************
* Copyright © 2024-2025 Light Zhang <mapaware@hotmail.com>, MapAware, Inc.     *
* ALL RIGHTS RESERVED.                                                         *
*                                                                              *
* PERMISSION IS HEREBY GRANTED, FREE OF CHARGE, TO ANY PERSON OR ORGANIZATION  *
* OBTAINING A COPY OF THE SOFTWARE COVERED BY THIS LICENSE TO USE, REPRODUCE,  *
* DISPLAY, DISTRIBUTE, EXECUTE, AND TRANSMIT THE SOFTWARE, AND TO PREPARE      *
* DERIVATIVE WORKS OF THE SOFTWARE, AND TO PERMIT THIRD - PARTIES TO WHOM THE  *
* SOFTWARE IS FURNISHED TO DO SO, ALL SUBJECT TO THE FOLLOWING :               *
*                                                                              *
* THE COPYRIGHT NOTICES IN THE SOFTWARE AND THIS ENTIRE STATEMENT, INCLUDING   *
* THE ABOVE LICENSE GRANT, THIS RESTRICTION AND THE FOLLOWING DISCLAIMER, MUST *
* BE INCLUDED IN ALL COPIES OF THE SOFTWARE, IN WHOLE OR IN PART, AND ALL      *
* DERIVATIVE WORKS OF THE SOFTWARE, UNLESS SUCH COPIES OR DERIVATIVE WORKS ARE *
* SOLELY IN THE FORM OF MACHINE - EXECUTABLE OBJECT CODE GENERATED BY A SOURCE *
* LANGUAGE PROCESSOR.                                                          *
*                                                                              *
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR   *
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,     *
* FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON - INFRINGEMENT.IN NO EVENT   *
* SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE    *
* FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,  *
* ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER  *
* DEALINGS IN THE SOFTWARE.                                                    *
*******************************************************************************/
/*
** @file      emerglog.h
**  syslog for EMERG event message.
**
**  see syslog as below:
**    $ cat /var/log/messages | grep $ident
**
** @author Liang Zhang <350137278@qq.com>
** @version 0.0.11
** @since 2017-05-02 12:02:50
** @date      2024-11-03 23:44:51
*/
#ifndef EMERG_LOG_H__
#define EMERG_LOG_H__

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


#ifndef EMERGLOG_MSGLEN_MAX
#   define EMERGLOG_MSGLEN_MAX   1023
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
    char msgbuf[EMERGLOG_MSGLEN_MAX + 1];
    int chlen = 0;

    va_list args;
    va_start(args, format);
    chlen = vsnprintf(msgbuf, sizeof(msgbuf), format, args);
    va_end(args);

    if (chlen > EMERGLOG_MSGLEN_MAX) {
        chlen = EMERGLOG_MSGLEN_MAX;
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
            emerg_syslog_message(EXIT_FAILURE, ident, __FILE__, __LINE__, message, ##__VA_ARGS__); \
        } \
    } while(0)

    # define emerglog_exit(ident, message, ...) \
        emerg_syslog_message(EXIT_FAILURE, ident, __FILE__, __LINE__, message, ##__VA_ARGS__)

    # define emerglog_msg(ident, message, ...) \
        emerg_syslog_message(0, ident, __FILE__, __LINE__, message, ##__VA_ARGS__)

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


/**
 * error message api
 */
#if defined(__WINDOWS__) || defined(__CYGWIN__)
NOWARNING_UNUSED(static)
const char* format_win32_syserror(int werrnum, char* errmsgbuf, size_t bufsize)
{
    int len;
    *errmsgbuf = '\0';
    len = (int)FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, (DWORD)werrnum, 0, errmsgbuf, (DWORD)bufsize, NULL);
    if (!len) {
        snprintf(errmsgbuf, bufsize, "Failed to FormatMessageA for Windows Error(%d): LastError=%d", werrnum, (int)GetLastError());
    }
    else if (len < (int)bufsize) {
        errmsgbuf[len] = '\0';
    }
    errmsgbuf[bufsize - 1] = '\0';
    return errmsgbuf;
}

#define format_syserror format_win32_syserror
#else
NOWARNING_UNUSED(static)
const char* format_posix_syserror(int errnum, char* errmsgbuf, size_t bufsize)
{
    char* str = errmsgbuf;
    *errmsgbuf = '\0';

    /* GNU-specific */
    str = strerror_r(errnum, errmsgbuf, bufsize);

    errmsgbuf[bufsize - 1] = '\0';
    return str;
}
#define format_syserror format_posix_syserror
#endif

#if defined(__cplusplus)
}
#endif

#endif /* EMERG_LOG_H__ */
