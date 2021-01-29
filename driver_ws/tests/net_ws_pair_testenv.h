#ifndef TEST_NET_WS_PAIR_TESTENV_H_INCLEDED
#define TEST_NET_WS_PAIR_TESTENV_H_INCLEDED
#include "net_acceptor.h"
#include "net_ws_testenv.h"

typedef struct net_ws_pair_testenv * net_ws_pair_testenv_t;

struct net_ws_pair_testenv {
    net_ws_testenv_t m_env;
    net_acceptor_t m_acceptor;
};

net_ws_pair_testenv_t net_ws_pair_testenv_create();
void net_ws_pair_testenv_free(net_ws_pair_testenv_t env);

net_endpoint_t net_ws_pair_testenv_get_svr_ep(
    net_ws_pair_testenv_t env, net_endpoint_t client_ep);
    
#endif
