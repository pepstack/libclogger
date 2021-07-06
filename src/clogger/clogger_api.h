/**
 * clogger_api.h
 */
#ifndef CLOGGER_API_H_PUBLIC
#define CLOGGER_API_H_PUBLIC

#ifdef    __cplusplus
extern "C" {
#endif

#include "clogger_def.h"


CLOGGER_API const char * clogger_lib_version(const char **_libname);


CLOGGER_API const char * logger_manager_version ();


/*!
 * @brief logger_manager_init
 *     Initialize logger manager by an optional config file.
 *     This function should be called in main entry of application.
 *
 *     YOU SHOULD NEVER CALL THIS METHOD IN ANY DLL (OR SO) MODULES.
 *
 * @param[in] logger_cfg   path file to a clooger config file.
 *     If 0 provided, clogger uses default search order for a logger.cfg.
 *     See clogger.cfg for more.
 *
 * @param[in] app_ident    optional ident for application logger.
 *     If provided, application logger can be retrieved anywhere by calling:
 *        logger_manager_load(NULL);
 */
CLOGGER_API void logger_manager_init (const char *logger_cfg, ...);


/*!
 * @brief logger_manager_uninit
 *
 *    YOU SHOULD NEVER CALL THIS METHOD IN ANY DLL (OR SO) MODULES.
 *
 *     Finalize all loggers within manager.
 *     This function should be called like the following:
 *         atexit(logger_manager_uninit);
 *
 */
CLOGGER_API void logger_manager_uninit (void);


CLOGGER_API logger_manager get_logger_manager();


/*!
 * @brief logger_manager_load
 *     Load a specified logger by ident name.
 *     clogger would create a logger for the ident and cache it.
 *     A Cached logger should be retrieved using logger_manager_get() function
 *       provided a loggerid returned by logger_manager_load().
 *
 * @param[in]     ident      name identifier for logger to load
 * @param[in]     ident      load clogger from logger_manager given it's ident.
 *   if it is NULL, get (app_ident) logger set when calling logger_manager_init.
 *
 * @return
 *    -<em>a clog_logger object</em> succeed
 *    -<em>0</em> fail
 */
CLOGGER_API clog_logger  logger_manager_load (const char *ident);


/*!
 * @brief logger_manager_get
 *     Get a cached logger by 1-based id.
 *     This method is fast enough and threads-safety.
 *
 * @param[in]     loggerid   an int identifier for cached logger.
 *             if 0 provided, get the first logger;
 *             if -1 provided, get the last logger;
 *        Get an id for logger uses:
 *             clog_logger_get_loggerid(logger);
 *
 * @param[in]     pvmgr      pointer to logger_manager returned by get_logger_manager()
 *
 * @return
 *    -<em>a clog_logger object</em> succeed
 *    -<em>0</em> fail
 */
CLOGGER_API clog_logger  logger_manager_get (int loggerid);


CLOGGER_API int logger_manager_get_stampid (char *stampidfmt, int fmtsize);


/**
 * logger conf api
 */
CLOGGER_API void logger_conf_init_default (clogger_conf conf, const char *ident, const char *pathprefix, const char *winsyslogconf);
CLOGGER_API void logger_conf_final_release (clogger_conf conf);
CLOGGER_API int  logger_conf_get_creatflags (const clogger_conf conf);
CLOGGER_API int  logger_conf_load_config (const char *cfgfile, const char *ident, clogger_conf conf);


/**
 * logger api
 */
CLOGGER_API clog_logger clog_logger_create (clogger_conf conf, logger_manager mgr);
CLOGGER_API void clog_logger_destroy (clog_logger logger);

CLOGGER_API int clog_logger_get_loggerid (clog_logger logger);
CLOGGER_API const char * clog_logger_get_ident (clog_logger logger);
CLOGGER_API int clog_logger_get_maxmsgsize (clog_logger logger);
CLOGGER_API int64_t clog_logger_get_logmessages (clog_logger logger, int64_t *round);
CLOGGER_API int clog_logger_level_enabled(clog_logger logger, clog_level_t level);
CLOGGER_API void clog_logger_log_message (clog_logger logger, clog_level_t level, uint16_t maxwaitms, const char *message, int msglen);
CLOGGER_API void clog_logger_log_format (clog_logger logger, clog_level_t level, uint16_t maxwaitms, const char *filename, int lineno, const char *funcname, const char *format, ...);


/**
 * real time clock api
 */
CLOGGER_API int clog_logger_get_timezone (clog_logger logger, const char **timezonefmt);

CLOGGER_API int clog_logger_get_daylight (clog_logger logger);

CLOGGER_API int64_t clog_logger_get_ticktime (clog_logger logger, struct timespec *ticktime);

CLOGGER_API void clog_logger_get_localtime (clog_logger logger, int timezone, int daylight, struct tm *tmloc, struct timespec *now);


/**
 * helper api
 */
CLOGGER_API const char * clog_logger_file_basename (const char *pathfile, int *namelen);
CLOGGER_API void clog_set_levelcolor (clog_logger logger, clog_level_t level, clog_color_t color);
CLOGGER_API void clog_set_levelstyle (clog_logger logger, clog_level_t level, clog_style_t style);
CLOGGER_API int clog_level_from_string (const char *levelstring, int length, clog_level_t *level);
CLOGGER_API int clog_layout_from_string (const char *layoutstring, int length, clog_layout_t *layout);
CLOGGER_API int clog_dateformat_from_string (const char *datefmtstring, int length, clog_dateformat_t *dateformat);
CLOGGER_API int clog_appender_from_string (const char *appenderstring, int length, int *appender);


#ifdef    __cplusplus
}
#endif

#endif /* CLOGGER_API_H_PUBLIC */