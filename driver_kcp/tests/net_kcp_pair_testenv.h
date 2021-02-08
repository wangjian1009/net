#ifndef TEST_NET_PCK_PAIR_TESTENV_H_INCLEDED
#define TEST_NET_PCK_PAIR_TESTENV_H_INCLEDED
#include "net_acceptor.h"
#include "net_kcp_testenv.h"

typedef struct net_kcp_pair_testenv * net_kcp_pair_testenv_t;

struct net_kcp_pair_testenv {
    net_kcp_testenv_t m_env;
    net_acceptor_t m_acceptor;
};

net_kcp_pair_testenv_t net_kcp_pair_testenv_create();
void net_kcp_pair_testenv_free(net_kcp_pair_testenv_t env);

net_endpoint_t net_kcp_pair_testenv_get_svr_stream(
    net_kcp_pair_testenv_t env, net_endpoint_t client_ep);
    
#endif
