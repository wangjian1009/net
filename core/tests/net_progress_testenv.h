#ifndef TESTS_NET_CORE_PROGRESS_TESTENV_H_INCLEDED
#define TESTS_NET_CORE_PROGRESS_TESTENV_H_INCLEDED
#include "cpe/utils/buffer.h"
#include "net_core_testenv.h"
#include "net_progress.h"

typedef struct net_progress_testenv * net_progress_testenv_t;

struct net_progress_testenv {
    net_core_testenv_t m_env;

    struct mem_buffer m_buffer;
};

net_progress_testenv_t net_progress_testenv_create();
void net_progress_testenv_free(net_progress_testenv_t env);

net_progress_t
net_progress_testenv_run_cmd_read(net_progress_testenv_t env, const char * cmd);

#endif
