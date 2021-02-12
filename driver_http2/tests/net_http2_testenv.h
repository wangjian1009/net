#ifndef TEST_NET_HTTP2_TESTENV_H_INCLEDED
#define TEST_NET_HTTP2_TESTENV_H_INCLEDED
#include "test_memory.h"
#include "test_error.h"
#include "net_http2_types.h"
#include "net_http2_req.h"
#include "net_http2_endpoint.h"
#include "net_http2_stream_endpoint.h"
#include "test_net_driver.h"
#include "test_net_endpoint.h"
#include "test_net_protocol_endpoint.h"

typedef struct net_http2_testenv * net_http2_testenv_t;

struct net_http2_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
    net_schedule_t m_schedule;

    /*basic*/
    test_net_driver_t m_tdriver;
    net_http2_protocol_t m_protocol;

    /*stream*/
    net_protocol_t m_stream_test_protocol;
    net_http2_stream_driver_t m_stream_driver;
};

net_http2_testenv_t net_http2_testenv_create();
void net_http2_testenv_free(net_http2_testenv_t env);

/*basic*/
net_http2_endpoint_t net_http2_testenv_create_ep_svr(net_http2_testenv_t env);

/*stream*/
net_http2_stream_endpoint_t
net_http2_testenv_create_stream_ep(net_http2_testenv_t env);

net_http2_stream_endpoint_t
net_http2_testenv_create_stream_ep_cli(net_http2_testenv_t env, const char * host);

net_http2_stream_endpoint_t
net_http2_testenv_stream_ep_other(net_http2_testenv_t env, net_http2_stream_endpoint_t ep);

net_acceptor_t
net_http2_testenv_create_stream_acceptor(net_http2_testenv_t env, const char * address);

#endif
