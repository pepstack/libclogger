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
** @file loggermgr.c
**  clogger manager methods.
**
** @author     Liang Zhang <350137278@qq.com>
** @version 1.0.2
** @since      2019-12-17 21:53:05
** @date 2024-11-04 00:02:51
*/
#include "loggermgr_i.h"

static pthread_once_t once_initialized = PTHREAD_ONCE_INIT;

static shmhandle_t shmgr_handle = shmipc_invalid_handle;

static struct clogger_version_mgr_t clogger_singleton = {
        {CLOGGER_MAJOR_VERSION, CLOGGER_MINOR_VERSION},
        {__DATE__, __TIME__},
        0,
        NULL
    };


const char * logger_manager_version()
{
    static char verstr[16];
    snprintf(verstr, sizeof(verstr), "%d.%d.%d.%d",
         (int)(ub1)((0xFF00 & CLOGGER_MAJOR_VERSION) >> 8)
        ,(int)(ub1)((0x00FF & CLOGGER_MAJOR_VERSION))
        ,(int)(ub1)((0xFF00 & CLOGGER_MINOR_VERSION) >> 8)
        ,(int)(ub1)((0x00FF & CLOGGER_MINOR_VERSION)));
    verstr[15] = 0;
    return verstr;
}


const char * clogger_lib_version(const char **_libname)
{
    if (_libname) {
        *_libname = LIBCLOGGER_NAME;
    }
    return LIBCLOGGER_VER;
}


static void init_once_routine(void)
{
    shmkey_t shmkey;

#if defined(__WINDOWS__) || defined(__CYGWIN__)
    char keybuf[128];
    shmkey = shmipc_keygen(".clogger-shared-manager.shm", getprocessid(), keybuf, sizeof(keybuf));
#else
    cstrbuf keyfile = get_proc_pathfile();
    shmkey = shmipc_keygen(keyfile->str, getprocessid());
    if (shmkey == -1) {
        char errmsg[ERROR_STRING_LEN_MAX];
        emerglog_exit("libclogger", "shmipc_keygen error(%d): %s",
            errno, format_posix_syserror(errno, errmsg, sizeof(errmsg)));
    }
    cstrbufFree(&keyfile);
#endif

    clogger_singleton.pid = getprocessid();
    clogger_singleton.pvmgr = mem_alloc_zero(1, sizeof(struct logger_manager_t));

    shmgr_handle = shmipc_create(shmkey, &clogger_singleton, sizeof(clogger_singleton), 0, 0);
    if (shmgr_handle == shmipc_invalid_handle) {
        char errmsg[ERROR_STRING_LEN_MAX];
#if defined(__WINDOWS__) || defined(__CYGWIN__)
        emerglog_exit("libclogger", "shmipc_create error(%d): %s",
            errno, format_win32_syserror(errno, errmsg, sizeof(errmsg)));
#else
        emerglog_exit("libclogger", "shmipc_create error(%d): %s",
            errno, format_posix_syserror(errno, errmsg, sizeof(errmsg)));
#endif
    }
}


static logger_manager get_logger_manager_shared()
{
    struct clogger_version_mgr_t outmgr;

    shmkey_t shmkey;

#if defined(__WINDOWS__) || defined(__CYGWIN__)
    char keybuf[128];
    shmkey = shmipc_keygen(".clogger-shared-manager.shm", getprocessid(), keybuf, sizeof(keybuf));
#else
    cstrbuf keyfile = get_proc_pathfile();
    shmkey = shmipc_keygen(keyfile->str, getprocessid());
    if (shmkey == -1) {
        char errmsg[ERROR_STRING_LEN_MAX];
        emerglog_exit("libclogger", "shmipc_keygen error(%d): %s",
            errno, format_posix_syserror(errno, errmsg, sizeof(errmsg)));
    }
    cstrbufFree(&keyfile);
#endif

    bzero(&outmgr, sizeof(outmgr));

    if (shmipc_read(shmkey, &outmgr, sizeof(outmgr), 0) == -1) {
        emerglog_exit("libclogger", "shmipc_read error(%d)", errno);
    }

    /* check libclogger version and build time and pid */
    clogger_singleton.pid = getprocessid();
    if (! memcmp(&clogger_singleton, &outmgr, 32)) {
        return (logger_manager) outmgr.pvmgr;
    }
    return NULL;
}


static clog_logger  logger_manager_load_shared (logger_manager mgr, const char *ident)
{
    if (! ident) {
        return mgr->app_logger;
    } else {
        int maxloggerid;
        struct clogger_ident_t *elt;

        clog_logger logger = NULL;

    #ifdef DISABLE_THREAD_RWLOCK
        if (pthread_mutex_lock(&mgr->thrlock) != 0) {
            emerglog_exit("libclogger", "pthread_mutex_lock error(%d)", errno);
        }

        if (! mgr->cfgfile) {
            pthread_mutex_unlock(&mgr->thrlock);
            printf("(%s:%d %s) ERROR: config file not found.\n", __FILE__, __LINE__, __FUNCTION__);
            return NULL;
        }
    #else
        if (RWLockAcquire(&mgr->rwlock, RWLOCK_STATE_READ, 0) != 0) {
            emerglog_exit("libclogger", "RWLockAcquire error");
        }

        if (! mgr->cfgfile) {
            RWLockRelease(&mgr->rwlock, RWLOCK_STATE_READ);
            printf("(%s:%d %s) ERROR: config file not found.\n", __FILE__, __LINE__, __FUNCTION__);
            return NULL;
        }
    #endif

        /* uthash is not threads-safety! */
        HASH_FIND_STR(mgr->loggers, ident, elt);

        if (elt) {
            logger = elt->logger;

        #ifdef DISABLE_THREAD_RWLOCK
            pthread_mutex_unlock(&mgr->thrlock);
        #else
            RWLockRelease(&mgr->rwlock, RWLOCK_STATE_READ);
        #endif

            /* success get an existed logger */
            return logger;
        }

        /* not found, reload config file to find logger */
    #ifndef DISABLE_THREAD_RWLOCK
        RWLockRelease(&mgr->rwlock, RWLOCK_STATE_READ);

        /* re-lock for write */
        if (RWLockAcquire(&mgr->rwlock, RWLOCK_STATE_WRITE, 0) != 0) {
            emerglog_exit("libclogger", "RWLockAcquire failed");
        }

        /* refind logger again */
        HASH_FIND_STR(mgr->loggers, ident, elt);

        if (elt) {
            logger = elt->logger;

            RWLockRelease(&mgr->rwlock, RWLOCK_STATE_WRITE);

            /* success get an existed logger */
            return logger;
        }
    #endif

        /* logger not found and we got write lock here */
        maxloggerid = ptr_cast_to_int(mgr->idloggers[0]);

        if (maxloggerid < CLOG_LOGGERID_MAX) {
            logger_conf_t conf = {0};

            logger_conf_init_default(&conf, ident, CLOG_PATHPREFIX_DEFAULT, 0);

            elt = (struct clogger_ident_t *) mem_alloc_zero(1, sizeof(struct clogger_ident_t) + conf.ident->len + 1);

            elt->idlen = conf.ident->len;
            memcpy(elt->ident, conf.ident->str, elt->idlen);

            if (logger_conf_load_config(mgr->cfgfile->str, elt->ident, &conf) != 0) {
                logger_conf_final_release(&conf);

                mem_free(elt);

            #ifdef DISABLE_THREAD_RWLOCK
                pthread_mutex_unlock(&mgr->thrlock);
            #else
                RWLockRelease(&mgr->rwlock, RWLOCK_STATE_WRITE);
            #endif

                return NULL;
            }

            /* create a new logger and add into uthash */
            conf.loggerid = maxloggerid + 1;
            logger = clog_logger_create(&conf, mgr);
            if (logger) {
                elt->logger = logger;
                maxloggerid = clog_logger_get_loggerid(elt->logger);

                mgr->idloggers[maxloggerid] = elt->logger;
                mgr->idloggers[0] = (clog_logger) int_cast_to_ptr(maxloggerid);

                HASH_ADD_STR_LEN(mgr->loggers, ident, elt->idlen, elt);
            }
        }

    #ifdef DISABLE_THREAD_RWLOCK
        pthread_mutex_unlock(&mgr->thrlock);
    #else
        RWLockRelease(&mgr->rwlock, RWLOCK_STATE_WRITE);
    #endif

        return logger;
    }
}


static clog_logger logger_manager_get_shared (logger_manager mgr, int loggerid)
{
    clog_logger logger = NULL;

#ifdef DISABLE_THREAD_RWLOCK
    if (pthread_mutex_lock(&mgr->thrlock) == 0)
#else
    if (RWLockAcquire(&mgr->rwlock, RWLOCK_STATE_READ, 0) == 0)
#endif
    {
        if (loggerid == 0) {
            // get the first logger
            logger = mgr->idloggers[1];
            goto return_logger;
        }

        if (loggerid == -1) {
            // get the last logger
            int lastid = ptr_cast_to_int(mgr->idloggers[0]);
            logger = mgr->idloggers[lastid];
            goto return_logger;
        }

        if (loggerid > 0 && loggerid <= CLOG_LOGGERID_MAX) {
            // get logger by id
            logger = mgr->idloggers[loggerid];
            goto return_logger;
        }

return_logger:
    #ifdef DISABLE_THREAD_RWLOCK
        pthread_mutex_unlock(&mgr->thrlock);
    #else
        RWLockRelease(&mgr->rwlock, RWLOCK_STATE_READ);
    #endif
    }

    return logger;
}


/**
 * process-wide logger manager
 */
logger_manager get_logger_manager()
{
    logger_manager mgr = (logger_manager) uatomic_ptr_get(&clogger_singleton.pvmgr);
    if (! mgr) {
        mgr = get_logger_manager_shared();
        if (uatomic_ptr_set(&clogger_singleton.pvmgr, mgr)) {
            perror("unexpected");
        }
    }
    return mgr;
}


/**
 * clogger public api
 */
void logger_manager_init(const char *logger_cfg, ...)
{
    pthread_once(&once_initialized, init_once_routine);

    /* Operations performed after initialization. */
    logger_manager mgr = get_logger_manager();

    if (! uatomic_int_comp_exch(&mgr->initialized, 0, 1)) {
        int count = 0;
        cstrbuf idents[CLOG_LOGGERID_MAX] = {0};

        printf("[%s:%d %s] initialize logger_manager\n", __FILE__, __LINE__, __FUNCTION__);

        mgr->rtclock = rtclock_init(RTCLOCK_FREQ_SEC);

        do {
            const char *argp;
            va_list aplist;
            va_start(aplist, logger_cfg);
            argp = va_arg(aplist, const char *);
            while (argp && count < sizeof(idents)/sizeof(idents[0])) {
                idents[count++] = cstrbufNew(0, argp, -1);
                argp = va_arg(aplist, const char *);
            }
            va_end(aplist);
        } while(0);

    #ifdef DISABLE_THREAD_RWLOCK
        pthread_mutex_init(&mgr->thrlock, 0);

        if (pthread_mutex_lock(&mgr->thrlock) != 0) {
            emerglog_exit("libclogger", "pthread_mutex_lock error(%d)", errno);
        }
    #else
        RWLockInit(&mgr->rwlock);

        if (RWLockAcquire(&mgr->rwlock, RWLOCK_STATE_WRITE, 0) != 0) {
            emerglog_exit("libclogger", "RWLockAcquire error(%d)", errno);
        }
    #endif

        if (! mgr->cfgfile) {
    #if defined(__WINDOWS__) || defined(__MINGW__)
            mgr->cfgfile = find_config_pathfile(logger_cfg, "clogger.cfg", "CLOGGER_CONF", 0);
    #elif defined(__CYGWIN__)
            emerglog_exit("libclogger", "TODO: cygwin not support");
    #else
            mgr->cfgfile = find_config_pathfile(logger_cfg, "clogger.cfg", "CLOGGER_CONF", "/etc/clogger");
    #endif
        }

        if (rollingfile_exists(mgr->cfgfile)) {
            printf("[%s:%d %s] load config file: {%.*s}\n", __FILE__, __LINE__, __FUNCTION__, cstrbufGetLen(mgr->cfgfile), cstrbufGetStr(mgr->cfgfile));
        } else {
            emerglog_exit("libclogger", "config file not found: {%.*s}", cstrbufGetLen(mgr->cfgfile), cstrbufGetStr(mgr->cfgfile));
        }

    #ifdef DISABLE_THREAD_RWLOCK
        pthread_mutex_unlock(&mgr->thrlock);
    #else
        RWLockRelease(&mgr->rwlock, RWLOCK_STATE_WRITE);
    #endif

        if (count > 0) {
            int i;

            for (i = 0; i < count; i++) {
                cstrbuf ident = idents[i];

                printf("[%s:%d %s] logger_manager_load_shared: {%.*s}\n", __FILE__, __LINE__, __FUNCTION__, cstrbufGetLen(ident), cstrbufGetStr(ident));

                clog_logger logger = logger_manager_load_shared(mgr, ident->str);
                if (! logger) {
                    emerglog_exit("libclogger", "logger ident not found: {%.*s}", cstrbufGetLen(ident), cstrbufGetStr(ident));
                }

                if (i == 0) {
                    mgr->app_logger = logger;
                }

                cstrbufFree(&ident);
                idents[i] = NULL;
            }
        }
    }
}


void logger_manager_uninit(void)
{
    logger_manager mgr = get_logger_manager();

    if (uatomic_int_comp_exch(&mgr->initialized, 1, 0)) {
        struct clogger_ident_t *curr, *tmp;

        printf("[%s:%d %s] uninitialize logger_manager\n", __FILE__, __LINE__, __FUNCTION__);

    #ifdef DISABLE_THREAD_RWLOCK
        pthread_mutex_lock(&mgr->thrlock);
        pthread_mutex_destroy(&mgr->thrlock);
    #else
        RWLockAcquire(&mgr->rwlock, RWLOCK_STATE_WRITE, 0);
        RWLockUninit(&mgr->rwlock);
    #endif

        cstrbufFree(&mgr->cfgfile);

        // delete all loggers
        HASH_ITER(hh, mgr->loggers, curr, tmp) {
            HASH_DEL(mgr->loggers, curr);

            clog_logger_destroy(curr->logger);
            mem_free(curr);
        }

        rtclock_uninit(mgr->rtclock);

        bzero(&clogger_singleton, sizeof(clogger_singleton));
        mem_free(mgr);
        shmipc_destroy(&shmgr_handle);
    }
}


clog_logger  logger_manager_load(const char *ident)
{
    logger_manager mgr = get_logger_manager();
    return (mgr? logger_manager_load_shared(mgr, ident) : NULL);
}


clog_logger logger_manager_get (int loggerid)
{
    logger_manager mgr = get_logger_manager();
    return (mgr? logger_manager_get_shared(mgr, loggerid) : NULL);
}


int logger_manager_get_stampid (char *stampidfmt, int fmtsize)
{
    struct timespec ts;

    logger_manager mgr = get_logger_manager();
    rtclock_ticktime(mgr->rtclock, &ts);
    return snprintf(stampidfmt, fmtsize, "{%"PRId64".%09d}", (int64_t)ts.tv_sec, (int)ts.tv_nsec);
}