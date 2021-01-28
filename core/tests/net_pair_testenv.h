#ifndef TESTS_NET_CORE_PAIR_TESTENV_H_INCLEDED
#define TESTS_NET_CORE_PAIR_TESTENV_H_INCLEDED
#include "net_core_testenv.h"
#include "net_pair.h"

typedef struct net_pair_testenv * net_pair_testenv_t;

struct net_pair_testenv {
    net_core_testenv_t m_env;
};

net_pair_testenv_t net_pair_testenv_create();
void net_pair_testenv_free(net_pair_testenv_t env);

void net_pair_testenv_make_pair(net_pair_testenv_t env, net_endpoint_t ep[2]);
void net_pair_testenv_make_pair_established(net_pair_testenv_t env, net_endpoint_t ep[2]);

#endif
