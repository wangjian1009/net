#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_signal.h"
#include "argtable2.h"
#include "cpe/utils/service.h"
#include "ndt7_runner.h"

int main(int argc, char * argv[]) {
    struct arg_end * use_end = arg_end(50);
    void * use_argtable[] = {
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

    ndt7_runner_t runner = ndt7_runner_create(NULL);
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

        if (ndt7_runner_init_net(runner) != 0) goto COMPLETE;
        if (ndt7_runner_init_dns(runner) != 0) goto COMPLETE;
        if (ndt7_runner_init_ndt(runner) != 0) goto COMPLETE;

        // Setup signal handler
#if defined SIGPIPE
        signal(SIGPIPE, SIG_IGN);
#endif

#if defined SIGBREAK
        ndt7_runner_init_stop_sig(runner, SIGBREAK);
#endif

#if defined SIGINT
        ndt7_runner_init_stop_sig(runner, SIGINT);
#endif

        ndt7_runner_init_stop_sig(runner, SIGTERM);

        if (ndt7_runner_start(runner, net_ndt7_test_download_and_upload) != 0) goto COMPLETE;

        ndt7_runner_loop_run(runner);

        rv = 0;
    COMPLETE:
        if (runner) {
            ndt7_runner_free(runner);
            runner = NULL;
        }

#if WIN32 || __MINGW32__
        WSACleanup();
#endif
    } else if (common_nerrors == 0) {
        if (common_help->count) {
            goto PRINT_HELP;
        }
    } else {
        goto PRINT_HELP;
    }
    
    goto EXIT;

PRINT_HELP:
    printf("%s:\n", argv[0]);
    printf("usage 1: %s ", argv[0]);
    arg_print_syntax(stdout, use_argtable, "\n");
    printf("usage 2: %s ", argv[0]);
    arg_print_syntax(stdout, common_argtable, "\n");

EXIT:
    arg_freetable(use_argtable, sizeof(use_argtable) / sizeof(use_argtable[0]));
    arg_freetable(common_argtable, sizeof(common_argtable) / sizeof(common_argtable[0]));

    return rv;
}
