#ifndef TESTS_NET_PROTOCOL_HTTP_TESTENV_H_INCLEDED
#define TESTS_NET_PROTOCOL_HTTP_TESTENV_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "test_memory.h"
#include "test_error.h"
#include "test_net_endpoint.h"
#include "test_net_protocol_endpoint.h"
#include "net_http_types.h"

typedef struct net_http_testenv * net_http_testenv_t;
typedef struct net_http_testenv_response * net_http_testenv_response_t;

typedef TAILQ_HEAD(net_http_testenv_response_list, net_http_testenv_response) net_http_testenv_response_list_t;

struct net_http_testenv {
    test_error_monitor_t m_tem;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    test_net_driver_t m_tdriver;
    net_protocol_t m_tprotocol;
    net_http_protocol_t m_http_protocol;
    net_http_testenv_response_list_t m_responses;
    struct mem_buffer m_tmp_buffer;
};

net_http_testenv_t net_http_testenv_create();
void net_http_testenv_free(net_http_testenv_t env);

net_http_endpoint_t net_http_testenv_create_ep(net_http_testenv_t env);
net_http_endpoint_t net_http_testenv_create_ep_established(net_http_testenv_t env);

net_http_testenv_response_t net_http_testenv_req_commit(net_http_testenv_t env, net_http_req_t req);

const char * net_http_testenv_ep_recv_write(net_http_testenv_t env, net_http_endpoint_t ep);
int net_http_testenv_ep_send_response(net_http_testenv_t env, net_http_endpoint_t ep, const char * response);
    
#endif
