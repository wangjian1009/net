#ifndef TEST_NET_XKCP_TESTENV_H_INCLEDED
#define TEST_NET_XKCP_TESTENV_H_INCLEDED
#include "test_memory.h"
#include "test_error.h"
#include "net_xkcp_config.h"
#include "net_xkcp_endpoint.h"
#include "test_net_driver.h"
#include "test_net_endpoint.h"
#include "test_net_protocol_endpoint.h"

typedef struct net_xkcp_testenv * net_xkcp_testenv_t;

struct net_xkcp_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    net_protocol_t m_test_protocol;
    test_net_driver_t m_tdriver;
    net_xkcp_driver_t m_xkcp_driver;
};

net_xkcp_testenv_t net_xkcp_testenv_create();
void net_xkcp_testenv_free(net_xkcp_testenv_t env);

net_xkcp_endpoint_t net_xkcp_testenv_create_ep(net_xkcp_testenv_t env);
net_xkcp_endpoint_t net_xkcp_testenv_create_cli_ep(net_xkcp_testenv_t env, const char * host);

/*pair*/
void net_xkcp_testenv_cli_create_pair(
    net_xkcp_testenv_t env, net_xkcp_endpoint_t * cli, net_xkcp_endpoint_t * svr,
    const char * address);

void net_xkcp_testenv_cli_create_pair_established(
    net_xkcp_testenv_t env, net_xkcp_endpoint_t * cli, net_xkcp_endpoint_t  * svr,
    const char * address);

/*svr*/
net_xkcp_acceptor_t
net_xkcp_testenv_create_acceptor(
    net_xkcp_testenv_t env, const char * address,
    net_acceptor_on_new_endpoint_fun_t on_new_endpoint, void * on_new_endpoint_ctx);

net_xkcp_endpoint_t net_xkcp_testenv_create_stream(net_xkcp_testenv_t env);

#endif
