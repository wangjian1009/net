#ifndef TESTS_NET_PROTOCOL_HTTPS_TESTENV_H_INCLEDED
#define TESTS_NET_PROTOCOL_HTTPS_TESTENV_H_INCLEDED
#include "net_http_testenv.h"
#include "net_ssl_types.h"

typedef struct net_https_testenv * net_https_testenv_t;

struct net_https_testenv {
    net_http_testenv_t m_env;
    net_ssl_stream_driver_t m_ssl_driver;
};

net_https_testenv_t net_https_testenv_create();
void net_https_testenv_free(net_https_testenv_t env);

net_http_endpoint_t
net_https_testenv_create_ep(net_https_testenv_t env);

#endif
