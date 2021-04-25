#ifndef TESTS_NET_PROTOCOL_HTTP_TESTENV_H_INCLEDED
#define TESTS_NET_PROTOCOL_HTTP_TESTENV_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "test_memory.h"
#include "test_error.h"
#include "test_net_endpoint.h"
#include "test_net_protocol_endpoint.h"
#include "net_http_test_protocol.h"
#include "net_http_types.h"

typedef struct net_http_testenv * net_http_testenv_t;

struct net_http_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    test_net_driver_t m_tdriver;
    net_http_test_protocol_t m_http_protocol;
    net_http_endpoint_t m_http_endpoint;
    struct mem_buffer m_tmp_buffer;
};

net_http_testenv_t net_http_testenv_create();
void net_http_testenv_free(net_http_testenv_t env);

void net_http_testenv_create_ep(net_http_testenv_t env);
void net_http_testenv_create_ep_established(net_http_testenv_t env);

const char * net_http_testenv_ep_recv_write(net_http_testenv_t env);
int net_http_testenv_ep_send_response(net_http_testenv_t env, const char * response);

int net_http_testenv_send_response(net_http_testenv_t env, const char * data);

#endif
