#ifndef TESTS_NET_DRIVER_HTTP2_TESTENV_RESPONSE_H_INCLEDED
#define TESTS_NET_DRIVER_HTTP2_TESTENV_RESPONSE_H_INCLEDED
#include "cpe/utils/buffer.h"
#include "net_http2_testenv.h"
#include "net_http2_req.h"

struct net_http2_testenv_response {
    net_http2_testenv_t m_env;
    TAILQ_ENTRY(net_http2_testenv_response) m_next;
    uint32_t m_req_id;
    net_http2_req_t m_runing_req;
    net_http2_res_state_t m_state;
    net_http2_res_result_t m_result;

    uint16_t m_code;
    char * m_code_msg;

    uint16_t m_head_count;
    struct {
        char * m_name;
        char * m_value;
    } m_headers[128];
    
    struct mem_buffer m_body;
};

net_http2_testenv_response_t
net_http2_testenv_response_create(net_http2_testenv_t env, net_http2_req_t req);

void net_http2_testenv_response_free(net_http2_testenv_response_t response);

#endif
