#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_signal.h"
#include "argtable2.h"
#include "event2/event.h"
#include "cpe/utils/service.h"
#include "url_runner.h"

int main(int argc, char * argv[]) {
    struct arg_str * log_dir = arg_str0(NULL, "log-dir", NULL, "log file name");
    struct arg_str * method = arg_str0(NULL, "method", NULL, "method");
    struct arg_str * url = arg_str1(NULL, "url", NULL, "url");
    struct arg_end * use_end = arg_end(50);
    void * use_argtable[] = {
        method,
        url,
        log_dir,
        use_end
    };
    int use_nerrors;

    /*common*/
    struct arg_lit * common_help = arg_lit0(NULL, "help", "print this help and exit");
    struct arg_end * common_end = arg_end(20);
    void * common_argtable[] = { common_help, common_end };
    int common_nerrors;

    struct error_monitor_node em_node_buf;
    struct error_monitor em_buf;
    int rv;
    char * p;

    int i;
    for (i = 0; i < argc;) {
        if (argv[i][0] == 0) {
            memmove(argv + i, argv + i + 1, sizeof(argv[0]) * (argc - i - 1));
            argc--;
        } else {
            ++i;
        }
    }

    rv = -1;

    use_nerrors = arg_parse(argc, argv, use_argtable);
    common_nerrors = arg_parse(argc, argv, common_argtable);

    url_runner_t runner = url_runner_create(NULL);
    if (runner == NULL) return -1;

    error_monitor_t em = runner->m_em;
    
    if (use_nerrors == 0) {
#ifdef WIN32
        WSADATA wsaData;
#endif

#if WIN32 || __MINGW32__
        if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
            CPE_ERROR(em, "entry: WSAStartup Error !");
            return -1;
        }
        if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 0) {
            CPE_ERROR(em, "entry: WSAStartup invalid ver.");
            return -1;
        }
#endif

        if (url_runner_init_net(runner) != 0) goto COMPLETE;
        if (url_runner_set_mode(runner, url_runner_mode_internal) != 0) goto COMPLETE;

        // Setup signal handler
#if defined SIGPIPE
        signal(SIGPIPE, SIG_IGN);
#endif

#if defined SIGBREAK
        url_runner_init_stop_sig(runner, SIGBREAK);
#endif

#if defined SIGINT
        url_runner_init_stop_sig(runner, SIGINT);
#endif

        url_runner_init_stop_sig(runner, SIGTERM);

        if (url_runner_start(
                runner, method->count ? method->sval[0] : "get",
                url->sval[0], NULL) != 0)
        {
            return -1;
        }
        
        url_runner_loop_run(runner);

        rv = 0;
    COMPLETE:
        if (runner) {
            url_runner_free(runner);
            runner = NULL;
        }

#if WIN32 || __MINGW32__
        WSACleanup();
#endif
    }

    return 0;
}
