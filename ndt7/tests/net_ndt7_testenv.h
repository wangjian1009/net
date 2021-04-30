#ifndef TESTS_NET_NDT7_TESTENV_H_INCLEDED
#define TESTS_NET_NDT7_TESTENV_H_INCLEDED
#include "test_memory.h"
#include "test_error.h"
#include "test_net_endpoint.h"
#include "test_net_driver.h"
#include "net_ndt7_system.h"

typedef struct net_ndt7_testenv * net_ndt7_testenv_t;

struct net_ndt7_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    test_net_driver_t m_tdriver;
};

net_ndt7_testenv_t net_ndt7_testenv_create();
void net_ndt7_testenv_free(net_ndt7_testenv_t env);

#endif
