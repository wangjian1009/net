#ifndef TESTS_PROMETHEUS_HTTP_TESTENV_H_INCLEDED
#define TESTS_PROMETHEUS_HTTP_TESTENV_H_INCLEDED
#include "test_memory.h"
#include "test_error.h"
#include "test_http_svr_testenv.h"
#include "test_net_driver.h"
#include "prometheus_http_processor.h"

typedef struct prometheus_http_testenv * prometheus_http_testenv_t;

struct prometheus_http_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
    prometheus_manager_t m_manager;
    net_schedule_t m_net_schedule;
    test_net_driver_t m_net_driver;
    test_http_svr_testenv_t m_http_svr_env;
    prometheus_http_processor_t m_http_processor;
};

prometheus_http_testenv_t prometheus_http_testenv_create();
void prometheus_http_testenv_free(prometheus_http_testenv_t env);

net_http_req_t
prometheus_http_testenv_create_req(
    prometheus_http_testenv_t env, net_http_req_method_t method, const char * url);

#endif
