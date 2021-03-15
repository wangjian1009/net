#ifndef TESTS_NET_PROTOCOL_HTTP_TEST_UTILS_RESPONSE_H_INCLEDED
#define TESTS_NET_PROTOCOL_HTTP_TEST_UTILS_RESPONSE_H_INCLEDED
#include "cpe/utils/buffer.h"
#include "net_http_req.h"
#include "net_http_test_protocol.h"

struct net_http_test_response {
    net_http_test_protocol_t m_protocol;
    TAILQ_ENTRY(net_http_test_response) m_next;
    uint32_t m_req_id;
    net_http_req_t m_runing_req;
    net_http_res_state_t m_state;
    net_http_res_result_t m_result;

    uint16_t m_code;
    char * m_code_msg;

    uint16_t m_head_count;
    struct {
        char * m_name;
        char * m_value;
    } m_headers[128];
    
    struct mem_buffer m_body;
};

net_http_test_response_t
net_http_test_response_create(net_http_test_protocol_t protocol, net_http_req_t req);

void net_http_test_response_free(net_http_test_response_t response);

#endif
