#ifndef TEST_NET_XKCP_TESTENV_H_INCLEDED
#define TEST_NET_XKCP_TESTENV_H_INCLEDED
#include "test_memory.h"
#include "test_error.h"
#include "test_net_driver.h"
#include "test_net_dgram.h"
#include "test_net_protocol_endpoint.h"
#include "test_net_endpoint.h"
#include "net_endpoint.h"
#include "net_xkcp_config.h"
#include "net_xkcp_endpoint.h"
#include "net_xkcp_acceptor.h"
#include "net_xkcp_client.h"
#include "net_xkcp_connector.h"

typedef struct net_xkcp_testenv * net_xkcp_testenv_t;

struct net_xkcp_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    net_protocol_t m_test_protocol;
    test_net_driver_t m_tdriver;
    net_xkcp_driver_t m_xkcp_driver;
};

net_xkcp_testenv_t net_xkcp_testenv_create();
void net_xkcp_testenv_free(net_xkcp_testenv_t env);

void net_xkcp_endpoint_parse_config(
    net_xkcp_testenv_t env, net_xkcp_config_t config, const char * str_cfg);

net_xkcp_connector_t
net_xkcp_testenv_create_connector(
    net_xkcp_testenv_t env, const char * address, const char * config);

net_xkcp_acceptor_t
net_xkcp_testenv_create_acceptor(
    net_xkcp_testenv_t env, const char * address, const char * config);

net_xkcp_endpoint_t net_xkcp_testenv_create_ep(net_xkcp_testenv_t env);
net_xkcp_endpoint_t net_xkcp_testenv_create_cli_ep(net_xkcp_testenv_t env, const char * remote_address);

#endif
