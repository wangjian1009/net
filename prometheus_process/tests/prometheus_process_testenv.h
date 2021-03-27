#ifndef TESTS_PROMETHEUS_PROCESS_TESTENV_H_INCLEDED
#define TESTS_PROMETHEUS_PROCESS_TESTENV_H_INCLEDED
#include "test_memory.h"
#include "test_error.h"
#include "test_vfs_testenv.h"
#include "cpe/utils/buffer.h"
#include "prometheus_process_provider.h"

typedef struct prometheus_process_testenv * prometheus_process_testenv_t;

struct prometheus_process_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
    test_vfs_testenv_t m_vfs_env;
    prometheus_manager_t m_manager;
    prometheus_process_provider_t m_provider;
    struct mem_buffer m_dump_buffer;
};

prometheus_process_testenv_t prometheus_process_testenv_create();
void prometheus_process_testenv_free(prometheus_process_testenv_t env);

const char * prometheus_process_testenv_collect(prometheus_process_testenv_t env);

void prometheus_process_install_limits(prometheus_process_testenv_t env, const char * data);
void prometheus_process_install_stat(prometheus_process_testenv_t env, const char * data);

#endif
