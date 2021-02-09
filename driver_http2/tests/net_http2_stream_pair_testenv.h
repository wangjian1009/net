#ifndef TEST_NET_HTTP2_STREAM_PAIR_TESTENV_H_INCLEDED
#define TEST_NET_HTTP2_STREAM_PAIR_TESTENV_H_INCLEDED
#include "net_acceptor.h"
#include "net_http2_testenv.h"

typedef struct net_http2_stream_pair_testenv * net_http2_stream_pair_testenv_t;

struct net_http2_stream_pair_testenv {
    net_http2_testenv_t m_env;
    net_acceptor_t m_acceptor;
};

net_http2_stream_pair_testenv_t net_http2_stream_pair_testenv_create();
void net_http2_stream_pair_testenv_free(net_http2_stream_pair_testenv_t env);

net_endpoint_t net_http2_stream_pair_testenv_get_svr_stream(
    net_http2_stream_pair_testenv_t env, net_endpoint_t client_ep);
    
#endif
