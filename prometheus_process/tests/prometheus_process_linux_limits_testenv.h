#ifndef TESTS_PROMETHEUS_PROCESS_LIMITS_TESTENV_H_INCLEDED
#define TESTS_PROMETHEUS_PROCESS_LIMITS_TESTENV_H_INCLEDED
#include "prometheus_process_linux_limits_i.h"
#include "prometheus_process_testenv.h"

typedef struct prometheus_process_linux_limits_testenv * prometheus_process_linux_limits_testenv_t;

struct prometheus_process_linux_limits_testenv {
    prometheus_process_testenv_t m_env;
    uint32_t m_row_count;
    struct prometheus_process_linux_limits_row m_rows[100];
};

prometheus_process_linux_limits_testenv_t prometheus_process_linux_limits_testenv_create();
void prometheus_process_linux_limits_testenv_free(prometheus_process_linux_limits_testenv_t env);

int prometheus_process_linux_limits_testenv_load(prometheus_process_linux_limits_testenv_t env);

prometheus_process_linux_limits_row_t
prometheus_process_linux_limits_testenv_find_row(
    prometheus_process_linux_limits_testenv_t env, const char * limit);

#endif
