#ifndef TESTS_NET_PROTOCOL_HTTP_TEST_UTILS_PROTOCOL_H_INCLEDED
#define TESTS_NET_PROTOCOL_HTTP_TEST_UTILS_PROTOCOL_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/buffer.h"
#include "net_http_types.h"

typedef struct net_http_test_protocol * net_http_test_protocol_t;
typedef struct net_http_test_response * net_http_test_response_t;
typedef TAILQ_HEAD(net_http_test_response_list, net_http_test_response) net_http_test_response_list_t;

struct net_http_test_protocol {
    net_http_protocol_t m_protocol;
    net_http_test_response_list_t m_responses;
    struct mem_buffer m_tmp_buffer;
};

net_http_test_protocol_t net_http_test_protocol_create(net_schedule_t schedule, const char * name_postfix);
void net_http_test_protocol_free(net_http_test_protocol_t protocol);

net_http_test_response_t net_http_test_req_commit(net_http_test_protocol_t protocol, net_http_req_t req);

#endif
