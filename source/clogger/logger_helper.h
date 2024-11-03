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
** @file logger_helper.h
**  clogger api helper functions.
**
** @author     Liang Zhang <350137278@qq.com>
** @version 1.0.2
** @since      2019-12-17 21:53:05
** @date 2024-11-04 02:29:11
**
** @note
**   This file is NOT a part of libclogger.
**   You can change it as your will.
*/
#ifndef LOGGER_HELPER_API_H_
#define LOGGER_HELPER_API_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "clogger_api.h"

#ifndef CLOG_TRACE_MSGWAIT
# define CLOG_TRACE_MSGWAIT  CLOG_MSGWAIT_NOWAIT
#endif

#ifndef CLOG_DEBUG_MSGWAIT
# define CLOG_DEBUG_MSGWAIT  CLOG_MSGWAIT_NOWAIT
#endif

#ifndef CLOG_INFO_MSGWAIT
# define CLOG_INFO_MSGWAIT  CLOG_MSGWAIT_INSTANT
#endif

#ifndef CLOG_WARN_MSGWAIT
# define CLOG_WARN_MSGWAIT  CLOG_MSGWAIT_INSTANT
#endif

#ifndef CLOG_ERROR_MSGWAIT
# define CLOG_ERROR_MSGWAIT  CLOG_MSGWAIT_INFINITE
#endif

#ifndef CLOG_FATAL_MSGWAIT
# define CLOG_FATAL_MSGWAIT  CLOG_MSGWAIT_INFINITE
#endif


#if defined(_MSC_VER) // MSVC (Windows) =>

#define LOGGER_TRACE(logger, message, ...)  do { \
                if (clog_logger_level_enabled(logger, CLOG_LEVEL_TRACE)) { \
                    clog_logger_log_format(logger, CLOG_LEVEL_TRACE, CLOG_TRACE_MSGWAIT, __FILE__, __LINE__, __FUNCTION__, message, __VA_ARGS__); \
                } \
            } while(0)


#define LOGGER_DEBUG(logger, message, ...)  do { \
                if (clog_logger_level_enabled(logger, CLOG_LEVEL_DEBUG)) { \
                    clog_logger_log_format(logger, CLOG_LEVEL_DEBUG, CLOG_DEBUG_MSGWAIT, __FILE__, __LINE__, __FUNCTION__, message, __VA_ARGS__); \
                } \
            } while(0)


#define LOGGER_INFO(logger, message, ...)  do { \
                if (clog_logger_level_enabled(logger, CLOG_LEVEL_INFO)) { \
                    clog_logger_log_format(logger, CLOG_LEVEL_INFO, CLOG_INFO_MSGWAIT, __FILE__, __LINE__, __FUNCTION__, message, __VA_ARGS__); \
                } \
            } while(0)


#define LOGGER_WARN(logger, message, ...)  do { \
                if (clog_logger_level_enabled(logger, CLOG_LEVEL_WARN)) { \
                    clog_logger_log_format(logger, CLOG_LEVEL_WARN, CLOG_WARN_MSGWAIT, __FILE__, __LINE__, __FUNCTION__, message, __VA_ARGS__); \
                } \
            } while(0)


#define LOGGER_ERROR(logger, message, ...)  do { \
                if (clog_logger_level_enabled(logger, CLOG_LEVEL_ERROR)) { \
                    clog_logger_log_format(logger, CLOG_LEVEL_ERROR, CLOG_ERROR_MSGWAIT, __FILE__, __LINE__, __FUNCTION__, message, __VA_ARGS__); \
                } \
            } while(0)


#define LOGGER_FATAL(logger, message, ...)  do { \
                if (clog_logger_level_enabled(logger, CLOG_LEVEL_FATAL)) { \
                    clog_logger_log_format(logger, CLOG_LEVEL_FATAL, CLOG_FATAL_MSGWAIT, __FILE__, __LINE__, __FUNCTION__, message, __VA_ARGS__); \
                } \
            } while(0)


#define LOGGER_FATAL_EXIT(logger, message, ...)  do { \
                if (clog_logger_level_enabled(logger, CLOG_LEVEL_FATAL)) { \
                    clog_logger_log_format(logger, CLOG_LEVEL_FATAL, CLOG_FATAL_MSGWAIT, __FILE__, __LINE__, __FUNCTION__, message, __VA_ARGS__); \
                } \
                exit(EXIT_FAILURE); \
            } while(0)

#else  // GCC (Linux, MingW)

#define LOGGER_TRACE(logger, message, args...)  do { \
                if (clog_logger_level_enabled(logger, CLOG_LEVEL_TRACE)) { \
                    clog_logger_log_format(logger, CLOG_LEVEL_TRACE, CLOG_TRACE_MSGWAIT, __FILE__, __LINE__, __FUNCTION__, message, ##args); \
                } \
            } while(0)


#define LOGGER_DEBUG(logger, message, args...)  do { \
                if (clog_logger_level_enabled(logger, CLOG_LEVEL_DEBUG)) { \
                    clog_logger_log_format(logger, CLOG_LEVEL_DEBUG, CLOG_DEBUG_MSGWAIT, __FILE__, __LINE__, __FUNCTION__, message, ##args); \
                } \
            } while(0)


#define LOGGER_INFO(logger, message, args...)  do { \
                if (clog_logger_level_enabled(logger, CLOG_LEVEL_INFO)) { \
                    clog_logger_log_format(logger, CLOG_LEVEL_INFO, CLOG_INFO_MSGWAIT, __FILE__, __LINE__, __FUNCTION__, message, ##args); \
                } \
            } while(0)


#define LOGGER_WARN(logger, message, args...)  do { \
                if (clog_logger_level_enabled(logger, CLOG_LEVEL_WARN)) { \
                    clog_logger_log_format(logger, CLOG_LEVEL_WARN, CLOG_WARN_MSGWAIT, __FILE__, __LINE__, __FUNCTION__, message, ##args); \
                } \
            } while(0)


#define LOGGER_ERROR(logger, message, args...)  do { \
                if (clog_logger_level_enabled(logger, CLOG_LEVEL_ERROR)) { \
                    clog_logger_log_format(logger, CLOG_LEVEL_ERROR, CLOG_ERROR_MSGWAIT, __FILE__, __LINE__, __FUNCTION__, message, ##args); \
                } \
            } while(0)


#define LOGGER_FATAL(logger, message, args...)  do { \
                if (clog_logger_level_enabled(logger, CLOG_LEVEL_FATAL)) { \
                    clog_logger_log_format(logger, CLOG_LEVEL_FATAL, CLOG_FATAL_MSGWAIT, __FILE__, __LINE__, __FUNCTION__, message, ##args); \
                } \
            } while(0)


#define LOGGER_FATAL_EXIT(logger, message, args...)  do { \
                if (clog_logger_level_enabled(logger, CLOG_LEVEL_FATAL)) { \
                    clog_logger_log_format(logger, CLOG_LEVEL_FATAL, CLOG_FATAL_MSGWAIT, __FILE__, __LINE__, __FUNCTION__, message, ##args); \
                } \
                exit(EXIT_FAILURE); \
            } while(0)

#endif // <= GCC
///////////////////////////////////////////////////////////////////////


#ifdef __cplusplus
}
#endif
#endif /* LOGGER_HELPER_API_H_ */
