#ifndef TEST_NET_HTTP2_TESTENV_RECEIVER_H_INCLEDED
#define TEST_NET_HTTP2_TESTENV_RECEIVER_H_INCLEDED
#include "net_http2_testenv.h"

struct net_http2_testenv_receiver {
    net_http2_testenv_t m_env;
    TAILQ_ENTRY(net_http2_testenv_receiver) m_next;
    struct mem_buffer m_buffer;
};

net_http2_testenv_receiver_t net_http2_testenv_receiver_create(net_http2_testenv_t env);
void net_http2_testenv_receiver_free(net_http2_testenv_receiver_t receiver);

#endif
