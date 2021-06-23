/**
 * @file:
 *
 * @author     Liang Zhang <350137278@qq.com>
 * @version    0.0.1
 * @create     2019-12-24 10:34:05
 * @update     2020-12-02 14:19:05
 */
#include <clogger/logger_api.h>

#include <common/timeut.h>
#include <common/misc.h>
#include <common/emerglog.h>

/* using pthread or pthread-w32 */
#include <sched.h>
#include <pthread.h>

#ifdef __WINDOWS__
    # include <common/win32/getoptw.h>

    # if !defined(__MINGW__)
        // link to pthread-w32 lib for MS Windows
        #pragma comment(lib, "pthreadVC2.lib")

        // link to libclogger.lib for MS Windows
        #pragma comment(lib, "libclogger.lib")
    # endif
#else
    // Linux: see Makefile
    # include <getopt.h>
#endif


#define  APPNAME     "logapp"
#define  APPVER      "1.0.0"


#define  APP_THREADS_MAX   100
#define  APP_MESSAGES_MAX  1000000000


typedef struct
{
    int threadno;
} app_threadarg_t;


void run_log();