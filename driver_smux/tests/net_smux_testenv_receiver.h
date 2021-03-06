#ifndef TEST_NET_SMUX_TESTENV_RECEIVER_H_INCLEDED
#define TEST_NET_SMUX_TESTENV_RECEIVER_H_INCLEDED
#include "net_smux_testenv.h"

struct net_smux_testenv_receiver {
    net_smux_testenv_t m_env;
    TAILQ_ENTRY(net_smux_testenv_receiver) m_next;
    struct mem_buffer m_buffer;
};

net_smux_testenv_receiver_t
net_smux_testenv_receiver_create(net_smux_testenv_t env, net_smux_stream_t stream);
void net_smux_testenv_receiver_free(net_smux_testenv_receiver_t receiver);

#endif
