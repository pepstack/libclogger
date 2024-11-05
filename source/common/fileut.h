/******************************************************************************
* Copyright © 2024-2035 Light Zhang <mapaware@hotmail.com>, MapAware, Inc.
* ALL RIGHTS RESERVED.
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in
* all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
******************************************************************************/
/*
** @file      fileut.h
** @brief file utility api.
**
** @author mapaware@hotmail.com
** @copyright © 2024-2030 mapaware.top All Rights Reserved.
** @version 0.0.31
**
** @since 2019-09-30 12:37:44
** @date      2024-11-03 23:43:27
**
** @note
*/
#ifndef _FILE_UT_H__
#define _FILE_UT_H__

#if defined(__cplusplus)
extern "C"
{
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "cstrbuf.h"

#define ERROR_STRING_LEN_MAX   1024

#if defined(__WINDOWS__)
    typedef HANDLE filehandle_t;

    # define filehandle_invalid INVALID_HANDLE_VALUE
    # define fseek_pos_set    ((int)FILE_BEGIN)
    # define fseek_pos_cur    ((int)FILE_CURRENT)
    # define fseek_pos_end    ((int)FILE_END)

    # define getprocessid()  ((int)GetCurrentProcessId())
    # define getthreadid()   ((int) GetCurrentThreadId())

    /* Dir API on Win32: https://blog.csdn.net/wangdq_1989/article/details/44985799 */
    # include <direct.h>    /* _getcwd */

#else /* non-windows: Linux or Cygwin */

    /* See feature_test_macros(7) */
    #ifndef _LARGEFILE64_SOURCE
    # define _LARGEFILE64_SOURCE
    #endif

    # include <sys/types.h>
    # include <sys/stat.h>
    # include <sys/time.h>
    # include <fcntl.h>
    # include <limits.h>
    # include <unistd.h>    /* usleep(), getcwd */

    # if defined(__CYGWIN__)
        #   include <Windows.h>
        #   include <pthread.h>    /* pthread_self() */
        NOWARNING_UNUSED(static) pid_t getthreadid(void)
        {
            pthread_t tid = pthread_self();
            return (int)ptr_cast_to_int(tid);
        }
    # else /* __linux */
        # include <sys/syscall.h> /* syscall(SYS_gettid) */

        /* Dir API on Win32: https://blog.csdn.net/wangdq_1989/article/details/44985799 */
        # include <dirent.h>

        NOWARNING_UNUSED(static) pid_t getthreadid(void)
        {
            return syscall(SYS_gettid);
        }
    # endif

    # define getprocessid()   ((int)getpid())

    typedef int filehandle_t;
    # define filehandle_invalid ((filehandle_t)(-1))

    # define fseek_pos_set    ((int)SEEK_SET)
    # define fseek_pos_cur    ((int)SEEK_CUR)
    # define fseek_pos_end    ((int)SEEK_END)

#endif


#if defined(__WINDOWS__)

NOWARNING_UNUSED(static)
filehandle_t file_create(const char *pathname, int flags, int mode)
{
    filehandle_t hf = CreateFileA(pathname, (DWORD)(flags), FILE_SHARE_READ,
                                  NULL,
                                  CREATE_NEW,
                                  (DWORD)(mode),
                                  NULL);
    return hf;
}

NOWARNING_UNUSED(static)
filehandle_t file_open_read(const char *pathname)
{
    filehandle_t hf = CreateFileA(pathname,
                                  GENERIC_READ,
                                  FILE_SHARE_READ,
                                  NULL,
                                  OPEN_EXISTING,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);
    return hf;
}

NOWARNING_UNUSED(static)
filehandle_t file_write_new(const char *pathname)
{
    return file_create(pathname, GENERIC_WRITE, FILE_ATTRIBUTE_NORMAL);
}

NOWARNING_UNUSED(static)
int file_close(filehandle_t *phf)
{
    if (phf) {
        filehandle_t hf = *phf;

        if (hf != filehandle_invalid) {
            *phf = filehandle_invalid;

            if (CloseHandle(hf)) {
                return 0;
            }
        }
    }
    return (-1);
}

NOWARNING_UNUSED(static)
sb8 file_seek(filehandle_t hf, sb8 distance, int fseekpos)
{
    LARGE_INTEGER li;
    li.QuadPart = distance;
    if (SetFilePointerEx(hf, li, &li, fseekpos)) {
        return (sb8)li.QuadPart;
    }
    /* error */
    return (sb8)(-1);
}

NOWARNING_UNUSED(static)
sb8 file_size(filehandle_t hf)
{
    LARGE_INTEGER li;
    if (GetFileSizeEx(hf, &li)) {
        /* success */
        return (sb8)li.QuadPart;
    }

    /* error */
    return (sb8)(-1);
}

NOWARNING_UNUSED(static)
int file_readbytes(filehandle_t hf, char *bytesbuf, ub4 sizebuf)
{
    BOOL ret;
    DWORD cbread, cboffset = 0;

    while (cboffset != (DWORD)sizebuf) {
        ret = ReadFile(hf, (void *)(bytesbuf + cboffset), (DWORD)(sizebuf - cboffset), &cbread, NULL);

        if (!ret) {
            /* read on error: uses GetLastError() for more */
            return (-1);
        }

        if (cbread == 0) {
            /* reach to end of file */
            break;
        }

        cboffset += cbread;
    }

    /* success: actual read bytes */
    return (int)cboffset;
}

NOWARNING_UNUSED(static)
int file_writebytes(filehandle_t hf, const char *bytesbuf, ub4 bytestowrite)
{
    BOOL ret;
    DWORD cbwritten, cboffset = 0;

    while (cboffset != (DWORD)bytestowrite) {
        ret = WriteFile(hf, (const void *)(bytesbuf + cboffset), (DWORD)(bytestowrite - cboffset), &cbwritten, NULL);

        if (!ret) {
            /* write on error */
            return (-1);
        }

        cboffset += cbwritten;
    }

    /* success */
    return 0;
}

NOWARNING_UNUSED(static)
int pathfile_exists(const char *pathname)
{
    if (pathname) {
        WIN32_FIND_DATAA FindFileData;
        HANDLE handle = FindFirstFileA(pathname, &FindFileData);

        if (handle != INVALID_HANDLE_VALUE) {
            FindClose(handle);

            /* found */
            return 1;
        }
    }

    /* not found */
    return 0;
}

NOWARNING_UNUSED(static)
int pathfile_remove(const char *pathname)
{
    if (DeleteFileA(pathname)) {
        return 0;
    }

    return (-1);
}

NOWARNING_UNUSED(static)
int pathfile_move(const char *pathnameOld, const char *pathnameNew)
{
    return MoveFileA(pathnameOld, pathnameNew);
}

#else /* Linux? */

NOWARNING_UNUSED(static)
filehandle_t file_create(const char *pathname, int flags, int mode)
{
    int fd = open(pathname, flags | O_CREAT | O_EXCL, (mode_t)(mode));
    return fd;
}

NOWARNING_UNUSED(static)
filehandle_t file_open_read(const char *pathname)
{
    int fd = open(pathname, O_RDONLY | O_EXCL, S_IRUSR | S_IRGRP | S_IROTH);
    return fd;
}

NOWARNING_UNUSED(static)
filehandle_t file_write_new(const char *pathname)
{
    return file_create(pathname, O_WRONLY, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
}

NOWARNING_UNUSED(static)
int file_close(filehandle_t *phf)
{
    if (phf) {
        filehandle_t hf = *phf;

        if (hf != filehandle_invalid) {
            *phf = filehandle_invalid;

            return close(hf);
        }
    }
    return (-1);
}

NOWARNING_UNUSED(static)
sb8 file_seek(filehandle_t hf, sb8 distance, int fseekpos)
{
    #ifdef __CYGWIN__
        /* warning: cygwin */
        return  (sb8) lseek(hf, distance, fseekpos);
    #else
        return (sb8) lseek64(hf, (off64_t)distance, fseekpos);
    #endif
}

NOWARNING_UNUSED(static)
sb8 file_size(filehandle_t hf)
{
    #ifdef __CYGWIN__
        /* warning: cygwin */
        return  (sb8) lseek(hf, 0, SEEK_END);
    #else
        return (sb8) lseek64(hf, 0, SEEK_END);
    #endif
}

NOWARNING_UNUSED(static)
int file_readbytes(filehandle_t hf, char *bytesbuf, ub4 sizebuf)
{
    size_t cbread, cboffset = 0;

    while (cboffset != (size_t)sizebuf) {
        cbread = read(hf, (void *)(bytesbuf + cboffset), (size_t)(sizebuf - cboffset));

        if (cbread == -1) {
            /* read on error: uses strerror(errno) for more */
            return (-1);
        }

        if (cbread == 0) {
            /* reach to end of file */
            break;
        }

        cboffset += cbread;
    }

    /* success: actual read bytes */
    return (int)cboffset;
}

/* https://linux.die.net/man/2/write */
NOWARNING_UNUSED(static)
int file_writebytes(filehandle_t hf, const char *bytesbuf, ub4 bytestowrite)
{
    ssize_t cbret;
    off_t woffset = 0;

    while (woffset != (off_t)bytestowrite) {
        cbret = write(hf, (const void *)(bytesbuf + woffset), bytestowrite - woffset);

        if (cbret == -1) {
            /* error */
            return (-1);
        }

        if (cbret > 0) {
            woffset += cbret;
        }
    }

    /* success */
    return 0;
}

NOWARNING_UNUSED(static)
int pathfile_exists(const char *pathname)
{
    if (pathname && access(pathname, F_OK) != -1) {
        /* file exists */
        return 1;
    }

    /* file doesn't exist */
    return 0;
}

NOWARNING_UNUSED(static)
int pathfile_remove(const char *pathname)
{
    return remove(pathname);
}

NOWARNING_UNUSED(static)
int pathfile_move(const char *pathnameOld, const char *pathnameNew)
{
    return rename(pathnameOld, pathnameNew);
}

#endif


NOWARNING_UNUSED(static)
int getenv_with_prefix(const char *varName, const char *prefix, char *valBuf, size_t valBufSize)
{
    const char *env = getenv(varName);
    if (env) {
        int prelen = cstr_length(prefix, valBufSize);
        int envlen = cstr_length(env, valBufSize);
        if (prelen + envlen < (int)valBufSize) {
            return snprintf_chkd_V1(valBuf, valBufSize, "%.*s%.*s", prelen, prefix, envlen, env);
        } else {
            /* buffer is not enough */
            return -1;
        }
    }

    /* env not found */
    return 0;
}


NOWARNING_UNUSED(static)
cstrbuf get_proc_pathfile(void)
{
    int r, bufsize = 128;
    char *pathbuf = (char*) alloca(bufsize);

#if defined(__WINDOWS__)

    while ((r = (int)GetModuleFileNameA(0, pathbuf, (DWORD)bufsize)) >= bufsize) {
        bufsize += 128;
        pathbuf = (char*) alloca(bufsize);
    }

    if (r <= 0) {
        printf("GetModuleFileNameA failed.\n");
        exit(EXIT_FAILURE);
    }

#else

    while ((r = readlink("/proc/self/exe", pathbuf, bufsize)) >= (int)bufsize) {
        bufsize += 128;
        pathbuf = alloca(bufsize);
    }

    if (r <= 0) {
        printf("readlink failed: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }

#endif

    pathbuf[r] = '\0';

    return cstrbufNew(0, pathbuf, r);
}


/**
 * get_proc_abspath
 *   get absolute path for current process.
 */
NOWARNING_UNUSED(static)
cstrbuf get_proc_abspath(void)
{
    cstrbuf pathfile = get_proc_pathfile();

    char *p = strrchr(pathfile->str, '\\');
    char *q = strrchr(pathfile->str, '/');

    if (p && q) {
        (p > q)? (*p = 0) : (*q = 0);
    } else if (p) {
        *p = 0;
    } else if (q) {
        *q = 0;
    }

    pathfile->len = (ub4)strnlen(pathfile->str, pathfile->len);

    return pathfile;
}


/**
 * get_curr_work_dir
 *   get current work directory.
 */
NOWARNING_UNUSED(static)
cstrbuf get_curr_work_dir()
{
    cstrbuf newcsb = 0;
    char* cwd = 0;
    char cwdbuf[4096];

#if defined(_INC_DIRECT)
    // direct.h on windows
    cwd = _getcwd(cwdbuf, sizeof(cwdbuf));
#elif defined(_DIRENT_H)
    // dirent.h on linux
    cwd = getcwd(cwdbuf, sizeof(cwdbuf));
#else
  # error "Both direct.h and dirent.h not found"
#endif

    if (cwd) {
        newcsb = cstrbufNew(0, cwd, cstrbuf_error_size_len);
    }

    // if null: path is too long or out of memory
    return newcsb;
}


NOWARNING_UNUSED(static)
cstrbuf find_config_pathfile(const char *cfgpath, const char *cfgname, const char *envvarname, const char *etcconfpath)
{
    cstrbuf config = 0;
    cstrbuf dname = 0;
    cstrbuf tname = 0;
    cstrbuf pname = 0;

    tname = cstrbufCat(0, "/%s", cfgname);
    pname = cstrbufCat(0, "%c%s", PATH_SEPARATOR_CHAR, cfgname);

    if (cfgpath) {
        int pathlen = cstr_length(cfgpath, -1);
        if (cstr_endwith(cfgpath, pathlen, pname->str, (int)pname->len) ||
            cstr_endwith(cfgpath, pathlen, tname->str, (int)tname->len)) {
            config = cstrbufNew(0, cfgpath, pathlen);
        } else {
            char endchr = cfgpath[pathlen - 1];

            if (endchr == PATH_SEPARATOR_CHAR || endchr == '/') {
                config = cstrbufCat(0, "%.*s%.*s", pathlen, cfgpath, (int)pname->len - 1, cfgname);
            } else {
                config = cstrbufCat(0, "%.*s%c%.*s", pathlen, cfgpath, PATH_SEPARATOR_CHAR, (int)pname->len - 1, cfgname);
            }
        }

        printf("(fileut.h:%d) [%d] check config: %.*s\n", __LINE__, getprocessid(), cstrbufGetLen(config), cstrbufGetStr(config));
        goto finish_up;
    } else {
        char *p;
        dname = get_proc_abspath();

        // 1: "$(appbin_dir)/clogger.cfg"
        config = cstrbufConcat(dname, pname, 0);

        if (config) {
            printf("(fileut.h:%d) [%d] check config: %.*s\n", __LINE__, getprocessid(), cstrbufGetLen(config), cstrbufGetStr(config));
        } else {
            printf("(fileut.h:%d) [%d] null config\n", __LINE__, getprocessid());
        }

        if (pathfile_exists(config->str)) {
            goto finish_up;
        }
        cstrbufFree(&config);

        // 2: "$(appbin_dir)/conf/clogger.cfg"
        config = cstrbufCat(0, "%.*s%cconf%.*s", cstrbufGetLen(dname), cstrbufGetStr(dname), PATH_SEPARATOR_CHAR, cstrbufGetLen(pname), cstrbufGetStr(pname));
        printf("(fileut.h:%d) [%d] check config: %.*s\n", __LINE__, getprocessid(), cstrbufGetLen(config), cstrbufGetStr(config));
        if (pathfile_exists(config->str)) {
            goto finish_up;
        }

        // 3: "$appbindir/../conf/clogger.cfg"
        cstrbufTrunc(config, dname->len);
        p = strrchr(config->str, PATH_SEPARATOR_CHAR);
        if (p) {
            cstrbufTrunc(config, (ub4)(p - config->str));
            config = cstrbufCat(config, "%cconf%.*s", PATH_SEPARATOR_CHAR, cstrbufGetLen(pname), cstrbufGetStr(pname));

            if (config) {
                printf("(fileut.h:%d) [%d] check config: %.*s\n", __LINE__, getprocessid(), cstrbufGetLen(config), cstrbufGetStr(config));
            } else {
                printf("(fileut.h:%d) [%d] null config\n", __LINE__, getprocessid());
            }

            if (pathfile_exists(config->str)) {
                goto finish_up;
            }
        }
        cstrbufFree(&config);

        if (envvarname) {
            // 4: $CLOGGER_CONF=/path/to/clogger.cfg
            printf("(fileut.h:%d) [%d] check environment: %s\n", __LINE__, getprocessid(), envvarname);

            p = getenv(envvarname);
            if (p) {
                char endchr;
                config = cstrbufNew(cstr_length(p, -1) + pname->len + 1, p, -1);

                if (cstr_endwith(config->str, config->len, pname->str, (int)pname->len) ||
                    cstr_endwith(config->str, config->len, tname->str, (int)tname->len)) {
                    printf("(fileut.h:%d) [%d] check config: %.*s\n", __LINE__, getprocessid(), cstrbufGetLen(config), cstrbufGetStr(config));
                    goto finish_up;
                }

                endchr = config->str[config->len - 1];
                if (endchr == PATH_SEPARATOR_CHAR || endchr == '/') {
                    config = cstrbufCat(config, "%.*s", (int)pname->len - 1, cfgname);
                } else {
                    config = cstrbufCat(config, "%.*s", cstrbufGetLen(pname), cstrbufGetStr(pname));
                }

                printf("(fileut.h:%d) [%d] check config: %.*s\n", __LINE__, getprocessid(), cstrbufGetLen(config), cstrbufGetStr(config));
                goto finish_up;
            }
        }

        if (etcconfpath) {
            printf("(fileut.h:%d) [%d] check os path: %s\n", __LINE__, getprocessid(), etcconfpath);

            config = cstrbufCat(0, "%s%.*s", etcconfpath, cstrbufGetLen(pname), cstrbufGetStr(pname));
            printf("(fileut.h:%d) [%d] check config: %.*s\n", __LINE__, getprocessid(), cstrbufGetLen(config), cstrbufGetStr(config));
            goto finish_up;
        }
    }

finish_up:
    cstrbufFree(&tname);
    cstrbufFree(&pname);
    cstrbufFree(&dname);

    /* dont need to know whether config exists or not at all */
    return config;
}


NOWARNING_UNUSED(static)
int path_is_abspath(char first, char second)
{
    // relative path
    // ./
    // ../
    if (first == '.') {
        return 0;
    }

    // absolute path
    //   linux: /home/...
    //   mingw: /c/...
    //   cygwin: /cygdrive/c/...
    if (first == '/') {
        return 1;
    }

    // absolute path for windows
    //   windows: C:\temp\...
    if (second == ':') {
        return 1;
    }

    // relative path
    return 0;
}

#ifdef __cplusplus
}
#endif
#endif /* _FILE_UT_H__ */
