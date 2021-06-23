/**
 * @filename   logapp.c
 *   sample application shows how to use libclogger.
 *
 * @author     Liang Zhang <350137278@qq.com>
 * @version    0.0.1
 * @create     2019-12-24 10:34:05
 * @update     2020-12-02 14:19:05
 */
#include "app_incl.h"

cstrbuf config = 0;
cstrbuf ident = 0;

int threads = 1;
ub8 messages = 10;
int microsecond = 0;


static void appexit_cleanup(void)
{
    logger_manager_uninit();

    cstrbufFree(&config);
    cstrbufFree(&ident);

    printf("[%s:%d] exit cleanup.\n", APPNAME, getprocessid());
}


void print_usage (void)
{
#if defined(__WINDOWS__) || defined(__CYGWIN__)
    fprintf(stdout, "Usage: %s.exe [Options...] \n", APPNAME);
#else
    fprintf(stdout, "Usage: %s [Options...] \n", APPNAME);
#endif

    fprintf(stdout, "  %s is a clog test tool.\n", APPNAME);

    fprintf(stdout, "Options:\n");
    fprintf(stdout, "  -h, --help                  display help information.\n");
    fprintf(stdout, "  -V, --version               show %s version.\n\n", APPNAME);
    fprintf(stdout, "\n");
    fprintf(stdout, "  -I, --ident=NAME            name for clogger identifer. ('%s' default)\n", APPNAME);
    fprintf(stdout, "  -C, --config=<CFGFILE>      initialize with config (CFGFILE or default).\n");
    fprintf(stdout, "  -t, --threads=NUM           number of threads. ('1' default)\n");
    fprintf(stdout, "  -n, --messages=NUM          number of messages. ('10' default)\n");
    fprintf(stdout, "  -u, --microsecond=USEC      sleep for microsecond. ('0' default)\n");
    fprintf(stdout, "  -D, --daemon                runs in background. (not default)\n");

    fflush(stdout);
}


int main (int argc, const char *argv[])
{
    WINDOWS_CRTDBG_ON

    int opt, optindex, background = 0;

    ident = cstrbufNew(0, APPNAME, -1);

    const struct option lopts[] = {
        {"help",           no_argument, 0, 'h'},
        {"version",        no_argument, 0, 'V'},
        {"ident",          required_argument, 0, 'I'},
        {"config",         optional_argument, 0, 'C'},
        {"threads",        required_argument, 0, 't'},
        {"messages",       required_argument, 0, 'n'},
        {"microsecond",    required_argument, 0, 'u'},
#ifndef __WINDOWS__
        {"daemon",         no_argument,       0, 'D'},
#endif
        {0, 0, 0, 0}
    };

    printf("[%s:%d] startup...\n", APPNAME, getprocessid());

    // read option args
    while ((opt = getopt_long_only(argc, (char *const *) argv,
#ifndef __WINDOWS__
        "hVDC::I:t:n:u:",
#else
        "hVC::I:t:n:u:",
#endif
        lopts, &optindex)) != -1) {
        switch (opt) {
        case '?':
            printf("error: specified option not found.\n");
            exit(EXIT_FAILURE);

        case 'h':
            print_usage();
            exit(0);
            break;
#ifndef __WINDOWS__
        case 'D':
            background = 1;
            break;
#endif
        case 'V':
        #ifdef NDEBUG
            fprintf(stdout, "%s-%s, Build Release: %s %s\n\n", APPNAME, APPVER, __DATE__, __TIME__);
        #else
            fprintf(stdout, "%s-%s, Build Debug: %s %s\n\n", APPNAME, APPVER, __DATE__, __TIME__);
        #endif
            exit(0);
            break;

        case 'I':     // ident
            ident = cstrbufDup(ident, optarg, -1);
            break;

        case 'C':     // config
            if (optarg) {
                config = cstrbufNew(ROF_PATHPREFIX_LEN_MAX + 1, optarg, -1);
            }
            break;

        case 't': // threads
            threads = atoi(optarg);
            if (threads < 1) {
                threads = 1;
            }
            if (threads > APP_THREADS_MAX) {
                threads = APP_THREADS_MAX;
            }
            break;

        case 'n': // messages
            messages = atoi(optarg);
            if (messages < 1) {
                messages = 1;
            }
            if (messages > APP_MESSAGES_MAX) {
                messages = APP_MESSAGES_MAX;
            }
            break;

        case 'u': // microsecond
            microsecond = atoi(optarg);
            if (microsecond < 0) {
                microsecond = 1000000;
            }
            if (microsecond > 1000000) {
                microsecond = 1000000;
            }
            break;
        }
    }

    if (background) {
#ifndef __WINDOWS__
        /* runs in background */
        printf("[%s] running as daemon: pid=%d\n", APPNAME, getpid());
        if (daemon(0, 0)) {
            perror("daemon error");
            exit(EXIT_FAILURE);
        }
#endif
    }

    /* initialize once for logger manager */
    if (config) {
        printf("[%s] load config: %.*s\n", APPNAME, config->len, config->str);

        logger_manager_init(config->str, ident->str, 0);
    } else {
        printf("[%s] load default config\n", APPNAME);

        logger_manager_init(NULL, ident->str, 0);
    }

    printf("[%s] logger manager version: %s\n", APPNAME, logger_manager_version());

    /* register on exit cleanup */
    atexit(appexit_cleanup);

    // load other idents as you need!

    /* test logger here */
    run_log();

    return 0;
}


static void * logapp_thread (void *arg)
{
    app_threadarg_t *threadarg = (app_threadarg_t *) arg;

    clog_logger logger =  logger_manager_load(NULL);

    int tid = threadarg->threadno;

    ub8 count = 0;

    time_t t1, t0 = time(0);

    printf("[thr-%d:%"PRIu64"] starting...\n", tid, messages);

    while (count < messages) {
        count++;

        LOGGER_TRACE(logger, "[%d:%lld] clogger is a high-performance, reliable, threads safety, easy to use, pure C logging library.", tid, count);

        LOGGER_DEBUG(logger, "[%d:%lld] As far as I know in the C world there was NO perfect logging facility for applications like logback in java or log4cxx in c++.", tid, count);

        LOGGER_INFO(logger, "[%d:%lld] Using printf can work, but can not be redirected or reformatted easily.", tid, count);

        LOGGER_WARN(logger, "[%d:%lld] syslog is slow and is designed for system use.", tid, count);

        LOGGER_ERROR(logger, "[%d:%lld] Others like LOG4C(has BUGs) or ZLOG(over-design) is somewhat of complication.", tid, count);

        LOGGER_FATAL(logger, "[%d:%lld] So I wrote CLOGGER from the bottom up!", tid, count);

        if (count % 10000 == 0) {
            t1 = time(0);
            printf("[thr-%d:%"PRIu64"] elapsed seconds=%d speed=%d/s.\n", tid, count, (int)(t1-t0), (int)(6*count/(t1 - t0 + 0.1)));
        }

        if (microsecond > 0) {
            sleep_usec(microsecond);
        }
    }

    t1 = time(0);
    printf("[thr-%d:%"PRIu64"] end. elapsed seconds=%d speed=%d/s.\n", tid, count, (int)(t1-t0), (int)(count/(t1 - t0 + 0.1)));

    free(threadarg);
    return (void*) 0;
}


void run_log ()
{
    int i;

    pthread_t tids[APP_THREADS_MAX] = {0};

    /* default */
    for (i = 0; i < threads; i++) {
        app_threadarg_t *threadarg = (app_threadarg_t *) malloc(sizeof(*threadarg));

        threadarg->threadno = i + 1;

        if (pthread_create(&tids[i], NULL, logapp_thread, (void*)threadarg) == -1) {
            printf("pthread_create failed.\n");
            exit(EXIT_FAILURE);
        }
    }

    for (i = 0; i < threads; i++) {
        int err = pthread_join(tids[i], NULL);
        if (err) {
            printf("pthread_join error: %s.\n", strerror(err));
            exit(EXIT_FAILURE);
        }
    }
}
