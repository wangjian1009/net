#ifndef TESTS_NET_PROTOCOL_HTTP_TEST_UTILS_PROTOCOL_H_INCLEDED
#define TESTS_NET_PROTOCOL_HTTP_TEST_UTILS_PROTOCOL_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/buffer.h"
#include "net_http_types.h"

typedef struct net_http_test_protocol * net_http_test_protocol_t;
typedef struct net_http_test_conn * net_http_test_conn_t;
typedef TAILQ_HEAD(net_http_test_conn_list, net_http_test_conn) net_http_test_conn_list_t;
typedef struct net_http_test_response * net_http_test_response_t;
typedef TAILQ_HEAD(net_http_test_response_list, net_http_test_response) net_http_test_response_list_t;

struct net_http_test_protocol {
    error_monitor_t m_em;
    net_http_protocol_t m_protocol;
    net_http_test_conn_list_t m_conns;
    net_http_test_response_list_t m_responses;
};

net_http_test_protocol_t
net_http_test_protocol_create(
    net_schedule_t schedule, error_monitor_t em, const char * name_postfix);
void net_http_test_protocol_free(net_http_test_protocol_t protocol);

net_http_endpoint_t
net_http_test_protocol_create_ep(net_http_test_protocol_t protocol, net_driver_t driver);

net_http_req_t
net_http_test_protocol_create_req(
    net_http_test_protocol_t protocol,
    net_driver_t driver, net_http_req_method_t method, const char * url);

net_http_test_response_t
net_http_test_protocol_req_commit(net_http_test_protocol_t protocol, net_http_req_t req);

#endif
