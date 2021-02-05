#ifndef TEST_NET_SSL_TESTENV_H_INCLEDED
#define TEST_NET_SSL_TESTENV_H_INCLEDED
#include "test_memory.h"
#include "test_error.h"
#include "net_ssl_types.h"
#include "test_net_driver.h"
#include "test_net_protocol_endpoint.h"
#include "net_ssl_endpoint.h"
#include "net_ssl_stream_endpoint.h"

typedef struct net_ssl_testenv * net_ssl_testenv_t;

struct net_ssl_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    net_protocol_t m_test_protocol;
    test_net_driver_t m_tdriver;
    net_ssl_protocol_t m_ssl_protocol;
    net_ssl_stream_driver_t m_stream_driver;
};

net_ssl_testenv_t net_ssl_testenv_create();
void net_ssl_testenv_free(net_ssl_testenv_t env);

/*stream*/
net_acceptor_t
net_ssl_testenv_create_stream_acceptor(
    net_ssl_testenv_t env, const char * address,
    net_acceptor_on_new_endpoint_fun_t on_new_endpoint, void * on_new_endpoint_ctx);

net_ssl_stream_endpoint_t net_ssl_testenv_create_stream_endpoint(net_ssl_testenv_t env);
net_ssl_stream_endpoint_t net_ssl_testenv_create_stream_cli_endpoint(net_ssl_testenv_t env, const char * remote_addr);
net_ssl_stream_endpoint_t net_ssl_testenv_create_stream_svr_endpoint(net_ssl_testenv_t env);

#endif
