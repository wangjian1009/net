#ifndef TESTS_NET_PROTOCOL_SMUX_TESTENV_H_INCLEDED
#define TESTS_NET_PROTOCOL_SMUX_TESTENV_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "test_memory.h"
#include "test_error.h"
#include "test_net_endpoint.h"
#include "net_smux_types.h"

typedef struct net_smux_testenv * net_smux_testenv_t;

struct net_smux_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    test_net_driver_t m_tdriver;
    net_smux_manager_t m_smux_manager;
};

net_smux_testenv_t net_smux_testenv_create();
void net_smux_testenv_free(net_smux_testenv_t env);

#endif
