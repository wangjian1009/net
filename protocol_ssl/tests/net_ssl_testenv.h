#ifndef TEST_NET_SSL_TESTENV_H_INCLEDED
#define TEST_NET_SSL_TESTENV_H_INCLEDED
#include "test_memory.h"
#include "test_error.h"
#include "net_ssl_types.h"
#include "net_ssl_cli_driver.h"
#include "test_net_driver.h"

typedef struct net_ssl_testenv * net_ssl_testenv_t;

struct net_ssl_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    test_net_driver_t m_tdriver;
    net_ssl_cli_driver_t m_cli_driver;
};

net_ssl_testenv_t net_ssl_testenv_create();
void net_ssl_testenv_free(net_ssl_testenv_t env);

/*cli*/
net_endpoint_t net_ssl_testenv_create_cli_ep(net_ssl_testenv_t env);

#endif
