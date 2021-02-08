#ifndef TEST_NET_KCP_TESTENV_H_INCLEDED
#define TEST_NET_KCP_TESTENV_H_INCLEDED
#include "test_memory.h"
#include "test_error.h"
#include "net_kcp_endpoint.h"
#include "test_net_driver.h"
#include "test_net_endpoint.h"
#include "test_net_protocol_endpoint.h"

typedef struct net_kcp_testenv * net_kcp_testenv_t;

struct net_kcp_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    net_protocol_t m_test_protocol;
    test_net_driver_t m_tdriver;
    net_kcp_driver_t m_kcp_driver;
};

net_kcp_testenv_t net_kcp_testenv_create();
void net_kcp_testenv_free(net_kcp_testenv_t env);

net_kcp_endpoint_t net_kcp_testenv_ep_create(net_kcp_testenv_t env);

net_kcp_endpoint_t
net_kcp_testenv_cli_ep_create(
    net_kcp_testenv_t env, const char * host);

/*pair*/
void net_kcp_testenv_cli_create_pair(
    net_kcp_testenv_t env, net_kcp_endpoint_t * cli, net_kcp_endpoint_t * svr,
    const char * address);

void net_kcp_testenv_cli_create_pair_established(
    net_kcp_testenv_t env, net_kcp_endpoint_t * cli, net_kcp_endpoint_t  * svr,
    const char * address);

/*svr*/
net_acceptor_t
net_kcp_testenv_create_acceptor(
    net_kcp_testenv_t env, const char * address,
    net_acceptor_on_new_endpoint_fun_t on_new_endpoint, void * on_new_endpoint_ctx);

net_kcp_endpoint_t net_kcp_testenv_create_stream(net_kcp_testenv_t env);

#endif
