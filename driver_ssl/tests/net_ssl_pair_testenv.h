#ifndef TEST_NET_SSL_PAIR_TESTENV_H_INCLEDED
#define TEST_NET_SSL_PAIR_TESTENV_H_INCLEDED
#include "net_acceptor.h"
#include "net_ssl_testenv.h"

typedef struct net_ssl_pair_testenv * net_ssl_pair_testenv_t;

struct net_ssl_pair_testenv {
    net_ssl_testenv_t m_env;
    net_acceptor_t m_acceptor;
};

net_ssl_pair_testenv_t net_ssl_pair_testenv_create();
void net_ssl_pair_testenv_free(net_ssl_pair_testenv_t env);

net_endpoint_t net_ssl_pair_testenv_get_svr_ep(
    net_ssl_pair_testenv_t env, net_endpoint_t client_ep);
    
#endif
