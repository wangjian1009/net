#ifndef TESTS_NET_PROTOCOL_SMUX_TESTENV_H_INCLEDED
#define TESTS_NET_PROTOCOL_SMUX_TESTENV_H_INCLEDED
#include "test_memory.h"
#include "test_error.h"
#include "test_net_endpoint.h"
#include "test_net_dgram.h"
#include "net_smux_config.h"
#include "net_smux_session.h"
#include "net_smux_dgram.h"
#include "net_smux_stream.h"

typedef struct net_smux_testenv * net_smux_testenv_t;
typedef struct net_smux_testenv_receiver * net_smux_testenv_receiver_t;
typedef TAILQ_HEAD(net_smux_testenv_receiver_list, net_smux_testenv_receiver) net_smux_testenv_receiver_list_t;

struct net_smux_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    test_net_driver_t m_tdriver;
    net_smux_protocol_t m_smux_protocol;
    struct net_smux_config m_smux_config;

    /*receiver*/
    net_smux_testenv_receiver_list_t m_receivers;
};

net_smux_testenv_t net_smux_testenv_create();
void net_smux_testenv_free(net_smux_testenv_t env);

net_smux_dgram_t
net_smux_testenv_create_dgram_svr(net_smux_testenv_t env, const char * address);

net_smux_dgram_t
net_smux_testenv_create_dgram_cli(net_smux_testenv_t env, const char * address);

net_smux_session_t
net_smux_testenv_dgram_open_session(
    net_smux_testenv_t env, net_smux_dgram_t dgram, const char * address);

net_smux_session_t
net_smux_testenv_dgram_find_session(
    net_smux_testenv_t env, net_smux_dgram_t dgram, const char * address);

net_smux_testenv_receiver_t
net_smux_testenv_create_stream_receiver(net_smux_testenv_t env, net_smux_stream_t stream);

#endif
