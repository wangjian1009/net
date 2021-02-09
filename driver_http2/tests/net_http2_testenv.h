#ifndef TEST_NET_HTTP2_TESTENV_H_INCLEDED
#define TEST_NET_HTTP2_TESTENV_H_INCLEDED
#include "test_memory.h"
#include "test_error.h"
#include "net_http2_types.h"
#include "net_http2_control_endpoint.h"
#include "net_http2_endpoint.h"
#include "test_net_driver.h"
#include "test_net_endpoint.h"
#include "test_net_protocol_endpoint.h"

typedef struct net_http2_testenv * net_http2_testenv_t;

struct net_http2_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    net_protocol_t m_test_protocol;
    test_net_driver_t m_tdriver;
    net_http2_control_protocol_t m_protocol;
    net_http2_driver_t m_driver;
};

net_http2_testenv_t net_http2_testenv_create();
void net_http2_testenv_free(net_http2_testenv_t env);

/*endpoint*/
net_http2_endpoint_t net_http2_testenv_svr_ep_create(net_http2_testenv_t env);

/*stream*/
net_http2_endpoint_t net_http2_testenv_stream_ep_create(net_http2_testenv_t env);

net_http2_endpoint_t
net_http2_testenv_stream_cli_ep_create(
    net_http2_testenv_t env, const char * host);

/*pair*/
void net_http2_testenv_cli_create_pair(
    net_http2_testenv_t env, net_http2_endpoint_t * cli, net_http2_endpoint_t * svr,
    const char * address);

void net_http2_testenv_cli_create_pair_established(
    net_http2_testenv_t env, net_http2_endpoint_t * cli, net_http2_endpoint_t  * svr,
    const char * address);

/*svr*/
net_acceptor_t
net_http2_testenv_create_acceptor(
    net_http2_testenv_t env, const char * address,
    net_acceptor_on_new_endpoint_fun_t on_new_endpoint, void * on_new_endpoint_ctx);

net_http2_endpoint_t net_http2_testenv_create_ep(net_http2_testenv_t env);

#endif
