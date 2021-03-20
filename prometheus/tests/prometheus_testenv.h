#ifndef TESTS_PROMETHEUS_TESTENV_H_INCLEDED
#define TESTS_PROMETHEUS_TESTENV_H_INCLEDED
#include "test_memory.h"
#include "test_error.h"
#include "prometheus_types.h"

typedef struct prometheus_testenv * prometheus_testenv_t;

struct prometheus_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
};

prometheus_testenv_t prometheus_testenv_create();
void prometheus_testenv_free(prometheus_testenv_t env);

#endif
