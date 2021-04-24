#ifndef TESTS_NET_PROTOCOL_HTTPS_TESTENV_H_INCLEDED
#define TESTS_NET_PROTOCOL_HTTPS_TESTENV_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "test_memory.h"
#include "test_error.h"
#include "test_net_endpoint.h"
#include "test_net_protocol_endpoint.h"
#include "net_http_test_protocol.h"
#include "net_http_types.h"
#include "net_ssl_types.h"

typedef struct net_https_testenv * net_https_testenv_t;

struct net_https_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    test_net_driver_t m_tdriver;
    net_http_test_protocol_t m_http_protocol;
    net_ssl_stream_driver_t m_ssl_driver;
    net_http_endpoint_t m_https_endpoint;
    net_ssl_stream_endpoint_t m_ssl_cli_endpoint;
    net_ssl_stream_endpoint_t m_ssl_svr_endpoint;
    struct mem_buffer m_tmp_buffer;
};

net_https_testenv_t net_https_testenv_create();
void net_https_testenv_free(net_https_testenv_t env);

net_http_endpoint_t net_https_testenv_create_ep(net_https_testenv_t env);
int net_https_testenv_send_response(net_https_testenv_t env, const char * data);

const char * net_https_testenv_dump_svr_received(mem_buffer_t buffer, net_https_testenv_t env);

#endif
