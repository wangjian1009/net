#ifndef TEST_NET_SSL_TESTENV_H_INCLEDED
#define TEST_NET_SSL_TESTENV_H_INCLEDED
#include "test_memory.h"
#include "test_error.h"
#include "net_ssl_types.h"
#include "net_ssl_cli_driver.h"
#include "net_ssl_svr_driver.h"
#include "test_net_driver.h"
#include "test_net_protocol_endpoint.h"

typedef struct net_ssl_testenv * net_ssl_testenv_t;

struct net_ssl_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    net_protocol_t m_test_protocol;
    test_net_driver_t m_tdriver;
    net_ssl_cli_driver_t m_cli_driver;
    net_ssl_svr_driver_t m_svr_driver;
};

net_ssl_testenv_t net_ssl_testenv_create();
void net_ssl_testenv_free(net_ssl_testenv_t env);

/*cli*/
net_endpoint_t net_ssl_testenv_cli_ep_create(net_ssl_testenv_t env);
net_endpoint_t net_ssl_testenv_cli_ep_undline(net_endpoint_t ep);

/*svr*/
net_acceptor_t
net_ssl_testenv_create_svr_acceptor(
    net_ssl_testenv_t env, const char * address,
    net_acceptor_on_new_endpoint_fun_t on_new_endpoint, void * on_new_endpoint_ctx);

#endif
