#ifndef TESTS_NET_PROTOCOL_HTTP_TESTENV_H_INCLEDED
#define TESTS_NET_PROTOCOL_HTTP_TESTENV_H_INCLEDED
#include "test_memory.h"
#include "test_error.h"
#include "test_net_endpoint.h"
#include "net_system.h"

typedef struct net_http_testenv * net_http_testenv_t;

struct net_http_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    test_net_driver_t m_tdriver;
};

net_http_testenv_t net_http_testenv_create();
void net_http_testenv_free(net_http_testenv_t env);

#endif
