#ifndef TEST_NET_WS_TESTENV_H_INCLEDED
#define TEST_NET_WS_TESTENV_H_INCLEDED
#include "test_memory.h"
#include "test_error.h"
#include "net_ws_types.h"
#include "net_ws_endpoint.h"
#include "net_ws_stream_endpoint.h"
#include "test_net_driver.h"
#include "test_net_endpoint.h"
#include "test_net_protocol_endpoint.h"

typedef struct net_ws_testenv * net_ws_testenv_t;

struct net_ws_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    net_protocol_t m_test_protocol;
    test_net_driver_t m_tdriver;
    net_ws_protocol_t m_protocol;
    net_ws_stream_driver_t m_stream_driver;
};

net_ws_testenv_t net_ws_testenv_create();
void net_ws_testenv_free(net_ws_testenv_t env);

/*endpoint*/
net_ws_endpoint_t net_ws_testenv_svr_ep_create(net_ws_testenv_t env);

/*stream*/
net_ws_stream_endpoint_t net_ws_testenv_stream_ep_create(net_ws_testenv_t env);

net_ws_stream_endpoint_t
net_ws_testenv_stream_cli_ep_create(
    net_ws_testenv_t env, const char * host, const char * path);

/*pair*/
void net_ws_testenv_cli_create_pair(
    net_ws_testenv_t env, net_ws_endpoint_t * cli, net_ws_endpoint_t * svr,
    const char * address, const char * path);

void net_ws_testenv_cli_create_pair_established(
    net_ws_testenv_t env, net_ws_endpoint_t * cli, net_ws_endpoint_t  * svr,
    const char * address, const char * path);

/*svr*/
net_acceptor_t
net_ws_testenv_create_acceptor(
    net_ws_testenv_t env, const char * address,
    net_acceptor_on_new_endpoint_fun_t on_new_endpoint, void * on_new_endpoint_ctx);

net_ws_stream_endpoint_t net_ws_testenv_create_stream(net_ws_testenv_t env);

#endif
