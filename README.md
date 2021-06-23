# clogger

![clogger](https://github.com/pepstack/clogger/blob/master/clogger-logo.png)

A high-performance, reliable, threads safety, easy to use, pure C logging library.

As far as I know, in the C world there was NO perfect logging library for applications like logback in java or log4cxx in c++. Using printf can work, but can not be redirected or reformatted easily. syslog is slow and is designed for system use. Others like LOG4C(has BUGs) or ZLOG(over-design) is somewhat of complication.

So I wrote CLOGGER from the bottom up!

clogger(since 5.0) using ringbufst.h.

ringbufst.h uses static memory allocation other than dynamic memory allocation that ringbuf.h does.

350137278@qq.com


## Features

- Supports multi platforms

 * Mingw (windows)

 * Cygwin (Windows)

 * Windows+pthread

 * Linux

- Threads and Process(spawn) Safety

- High throughput

- Color output

- Fully functional

- Multi-idents in one config

- Easy to use


## Build static lib

- Windows: libclogger.lib

    use vs2015 to open msvc/clogger/clogger.sln, and build all.
    
    all built results are copied into ./libclogger/

- Others: libclogger.a

See:

   $ make help

## Speed Test (Force to write successfully)

```
    Windows = Windows 7, Thinkpad x240

    Linux = Linux RHEL6.4 Server (24 cores  Intel(R) Xeon(R) CPU E5-2640 0 @ 2.50GHz)

    clogger.cfg: queuelength=1000, ROFILE

    w=10000

    ----------------------------------------------
        os        threads      avg.speed (msg/sec)
    ----------------------------------------------
     Windows + win32       1           26w~27w
     Windows + win32      10           25w~30w
     Windows + win32      20             ~25w

     Windows + cygwin      1           20w~22w
     Windows + cygwin     10            9w~10w
     Windows + cygwin     20              ~8w

     Linux                 1           30w~34w
     Linux                10           35w~40w
     Linux                20             ~38w

```


## Usage


```
/**
 * main.c
 *   your app using clogger.
 *
 * see: examples/logapp.c for more
 */
#include <clogger/logger_api.h>


#define APP_LOGGER_IDENT  "logapp"


int main (int argc, char *argv[])
{
    // logger_manager_init("/etc/clogger/");
    logger_manager_init(0 /* 0 is for default config file */, APP_LOGGER_IDENT, NULL);

    /* register on exit cleanup */
    atexit(logger_manager_uninit);

    /* load default app logger */
    clog_logger logger = logger_manager_load(NULL);

    /* see: logger_api.h
     * LOGGER_TRACE
     * LOGGER_DEBUG
     * LOGGER_INFO
     * LOGGER_WARN
     * LOGGER_ERROR
     * LOGGER_FATAL
     * LOGGER_FATAL_EXIT
     */

    LOGGER_INFO(logger, "my beautiful china!\n");
    LOGGER_WARN(logger, "clogger is process and multi-threads safety.\n");

    sleep_msec(1000);

    return 0;
}
```


## Memcheck


```
$ valgrind --leak-check=full --show-leak-kinds=all ./logapp -n1000 -t4

==5266== Memcheck, a memory error detector
==5266== Copyright (C) 2002-2017, and GNU GPL'd, by Julian Seward et al.
==5266== Using Valgrind-3.13.0 and LibVEX; rerun with -h for copyright info
==5266== Command: ./logapp -n1000 -t4
==5266== 
[logapp:5266] startup...
[logapp] load default config
[logger_api.c:169 logger_manager_init] check config: /home/cl/Workspace/github.com/clogger/clogger.cfg
[logger_api.c:269 logger_manager_init] SUCCESS: /home/cl/Workspace/github.com/clogger/clogger.cfg
[logapp] logger ident: logapp
[thr-3:1000] starting...
[thr-1:1000] starting...
[thr-2:1000] starting...
[thr-4:1000] starting...
[thr-1:1000] end. elapsed seconds=4 speed=243/s.
[thr-2:1000] end. elapsed seconds=4 speed=243/s.
[thr-3:1000] end. elapsed seconds=4 speed=243/s.
[thr-4:1000] end. elapsed seconds=4 speed=243/s.
[logapp:5266] exit cleanup.
==5266== 
==5266== HEAP SUMMARY:
==5266==     in use at exit: 0 bytes in 0 blocks
==5266==   total heap usage: 20,304 allocs, 20,304 frees, 24,769,991 bytes allocated
==5266== 
==5266== All heap blocks were freed -- no leaks are possible
==5266== 
==5266== For counts of detected and suppressed errors, rerun with: -v
==5266== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```


## Sample

![clogger](https://github.com/pepstack/clogger/blob/master/sample.png)


## clogger 用于库

假设动态库mylib中有个全局变量定义的.h文件。比如 mylib_i.h

```
/**
 * mylib_i.h
 */
#include <clogger/logger_api.h>

extern clog_logger logger;
```

在动态库的 .c (.cxx) 文件中，导入这个变量, 然后在动态库的初始化方法中设置 logger 的值：

```
/**
 * mylib.c
 */
#include "mylib_i.h"

clog_logger logger = NULL;


void mylib_init()
{
    logger = logger_manager_load(NULL /* or ident */);

    LOGGER_DEBUG(logger, "Initialize DLL");
}
```

动态库的其他任何 .c(cxx) 文件, 如 any.c, 可以直接使用 logger:

```
/**
 * any.c
 */
#include "mylib_i.h"

int method()
{
    LOGGER_DEBUG(logger, "method called");
}
```