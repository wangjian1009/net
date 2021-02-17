#ifndef TESTS_NET_PROTOCOL_SMUX_TESTENV_H_INCLEDED
#define TESTS_NET_PROTOCOL_SMUX_TESTENV_H_INCLEDED
#include "test_memory.h"
#include "test_error.h"
#include "test_net_endpoint.h"
#include "net_smux_session.h"
#include "net_smux_stream.h"

typedef struct net_smux_testenv * net_smux_testenv_t;

struct net_smux_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    test_net_driver_t m_tdriver;
    net_smux_protocol_t m_smux_protocol;
};

net_smux_testenv_t net_smux_testenv_create();
void net_smux_testenv_free(net_smux_testenv_t env);

net_smux_session_t
net_smux_testenv_create_session_udp_svr(net_smux_testenv_t env, const char * address);

net_smux_session_t
net_smux_testenv_create_session_udp_cli(net_smux_testenv_t env, const char * address);

#endif
