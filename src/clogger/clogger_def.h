/**
 * clogger_def.h
 */
#ifndef CLOGGER_DEF_H_INCLUDED
#define CLOGGER_DEF_H_INCLUDED

#if defined(__cplusplus)
extern "C" {
#endif

#include <stdint.h>
#include <time.h>


#if defined(CLOGGER_DLL)
/* win32 dynamic dll */
# ifdef CLOGGER_EXPORTS
#   define CLOGGER_API __declspec(dllexport)
# else
#   define CLOGGER_API __declspec(dllimport)
# endif
#else
/* static lib or linux so */
# define CLOGGER_API  extern
#endif


#if defined(_WIN32) || defined(__MINGW__)
    # define CLOG_PATH_SEPARATOR       '\\'
    # define CLOG_PATHPREFIX_DEFAULT   "C:\\TEMP\\clogger\\win32\\"
#elif defined (__CYGWIN__)
    # define CLOG_PATH_SEPARATOR       '/'
    # define CLOG_PATHPREFIX_DEFAULT   "/cygdrive/c/TEMP/clogger/cygwin/"
#else
    # define CLOG_PATH_SEPARATOR       '/'
    # define CLOG_PATHPREFIX_DEFAULT   "/var/log/clogger/"
#endif


/**
 * process-wide realtime clock
 */
typedef struct logger_manager_t  * logger_manager;
typedef struct _clog_logger_t    * clog_logger;
typedef struct _logger_conf_t    * clogger_conf;


/**
 * available patterns:
 *   <PID>          - process id
 *   <IDENT>        - ident name
 *   <DATE>         - date format
 */
#define CLOG_NAMEPATTERN_DEFAULT     "<IDENT>-<PID>.<DATE>.log"

/**
 * max error message length
 */
#define CLOG_ERRMSG_LEN_MAX          255

/* message buffer size */
#define CLOG_MSGBUF_SIZE_MIN         1000
#define CLOG_MSGBUF_SIZE_DEFAULT     4000

#ifndef CLOG_MSGBUF_SIZE_MAX
# define CLOG_MSGBUF_SIZE_MAX        32640
#endif

/* max and enough size for datetime format */
#define CLOG_DATEFMT_SIZE_MAX        48

/* wait forever to log one message */
#define CLOG_MSGWAIT_INFINITE        ((-1))

/* nowait to log one message */
#define CLOG_MSGWAIT_NOWAIT          0

/* wait interval in ms to push msg: 1 up to 50 is good */
#define CLOG_MSGWAIT_INSTANT         (1)

/**
 * bit flags configuration for logger
 */
/* appender type which can be combined */
#define CLOG_APPENDER_STDOUT         0x1
#define CLOG_APPENDER_SYSLOG         0x2
#define CLOG_APPENDER_ROFILE         0x4
#define CLOG_APPENDER_SHMMAP         0x8

/* rolling policy for rollingfile appender which can be combined */
#define CLOG_ROLLING_SIZE_BASED      0x10
#define CLOG_ROLLING_TIME_BASED      0x20

/* time accuracy unit using second (default): s
 *   format: YYYYMMDD-hhmmss+0800
 */
#define CLOG_TIMEUNIT_SEC            0x0

/* time accuracy unit using milli-second (default is second): ms
 *   format: YYYYMMDD-hhmmss.nnn+0800
 */
#define CLOG_TIMEUNIT_MSEC           0x40

/* time accuracy unit using micro-second: us = 1/1000 ms
 *   format: YYYYMMDD-hhmmss.nnnuuu GMT+8:00
 */
#define CLOG_TIMEUNIT_USEC           0x80

/* nanosecond based timestamp id for message
 *   sample: {1576463937.089604916}
 */
#define CLOG_TIMESTAMP_ID            0x100

/* default to use UTC(+0000) timezone */
#define CLOG_TIMEZONE_LOC            0x200

/* levelcolors enable flag */
#define CLOG_LEVEL_COLORS            0x400

/* levelstyles enable flag */
#define CLOG_LEVEL_STYLES            0x800

/* __FILE__:__LINE__ */
#define CLOG_FILE_LINENO             0x1000

/* __FUNCTION__ */
#define CLOG_FUNCTION_NAME           0x2000


/**
 * never change below lines
 */
#define ROF_PATHPREFIX_LEN_MAX       248
#define ROF_NAMEPATTERN_LEN_MAX      120

#define ROF_DATE_SYMBOL              "<DATE>"
#define ROF_DATE_SYMBOLLEN           6

#define ROF_IDENT_SYMBOL             "<IDENT>"
#define ROF_IDENT_SYMBOLLEN          7

#define ROF_PID_SYMBOL               "<PID>"
#define ROF_PID_SYMBOLLEN            5

#define ROF_DATEMINUTE_SIZE          16

/* max file is up to: 64 GB */
#define ROF_MAXFILESIZE            ((uint64_t)68719476736UL)

/* max count of files is up to: 1,000,000 */
#define ROF_MAXFILECOUNT           1000000


typedef enum {
    CLOG_LAYOUT_PLAIN = 0,
    CLOG_LAYOUT_DATED = 1
} clog_layout_t;


typedef enum {
    CLOG_DATEFMT_RFC_3339  = 0,    /* `date --rfc-3339=seconds` = "2019-12-26 10:13:41+08:00" */
    CLOG_DATEFMT_ISO_8601  = 1,    /* `date --iso-8601=seconds  = "2019-12-26T10:14:32+08:00" */
    CLOG_DATEFMT_RFC_2822  = 2,    /* `date --rfc-2822`         = "Thu, 26 Dec 2019 10:12:45 +0800" */
    CLOG_DATEFMT_UNIVERSAL = 3,    /* `date -u,--universal`     = "Thu Dec 26 02:16:02 UTC 2019" */
    CLOG_DATEFMT_NUMERIC_2 = 4,    /*                           = "20191226-101245+0800" */
    CLOG_DATEFMT_NUMERIC_1 = 5     /*                           = "20191226101245+0800" */
} clog_dateformat_t;


typedef enum {
    CLOG_LEVEL_OFF    = 0,
    CLOG_LEVEL_FATAL  = 4,
    CLOG_LEVEL_ERROR  = 5,
    CLOG_LEVEL_WARN   = 6,
    CLOG_LEVEL_INFO   = 7,
    CLOG_LEVEL_DEBUG  = 8,
    CLOG_LEVEL_TRACE  = 9,
    CLOG_LEVEL_ALL    = 10
} clog_level_t;


/**
 * "\033[0;31m RED         \033[0m"
 * "\033[1;31m LIGHT RED   \033[0m"
 * "\033[0;32m GREEN       \033[0m"
 * "\033[3;32m LIGHT GREEN \033[0m"
 */
typedef enum {
    CLOG_STYLE_NORMAL     = 0,
    CLOG_STYLE_BOLD       = 1,
    CLOG_STYLE_LIGHT      = CLOG_STYLE_BOLD,
    CLOG_STYLE_DIM        = 2,
    CLOG_STYLE_ITALIC     = 3,
    CLOG_STYLE_UNDERLINED = 4,
    CLOG_STYLE_BLINKING   = 5,
    CLOG_STYLE_REVERSE    = 7,
    CLOG_STYLE_INVISIBLE  = 8
} clog_style_t;

typedef enum {
    CLOG_COLOR_NOCLR   = 0,
    CLOG_COLOR_DARK    = 30,
    CLOG_COLOR_RED     = 31,
    CLOG_COLOR_GREEN   = 32,
    CLOG_COLOR_YELLOW  = 33,
    CLOG_COLOR_BLUE    = 34,
    CLOG_COLOR_PURPLE  = 35,
    CLOG_COLOR_CYAN    = 36,
    CLOG_COLOR_WHITE   = 37
} clog_color_t;

typedef enum {
    ROLLING_TM_NONE   = 0,
    ROLLING_TM_MIN_1  = 1,
    ROLLING_TM_MIN_5  = 2,
    ROLLING_TM_MIN_10 = 3,
    ROLLING_TM_MIN_30 = 4,
    ROLLING_TM_HOUR   = 5,
    ROLLING_TM_DAY    = 6,
    ROLLING_TM_MON    = 7,
    ROLLING_TM_YEAR   = 8
} rollingtime_t;


#if defined(__cplusplus)
}
#endif

#endif /* CLOGGER_DEF_H_INCLUDED */