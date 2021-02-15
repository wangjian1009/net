#ifndef NET_HTTP2_REQ_I_H_INCLEDED
#define NET_HTTP2_REQ_I_H_INCLEDED
#include "net_http2_req.h"
#include "net_http2_endpoint_i.h"

NET_BEGIN_DECL

struct net_http2_req_pair {
    char * m_name;
    char * m_value;
};

struct net_http2_req {
    net_http2_endpoint_t m_endpoint;
    TAILQ_ENTRY(net_http2_req) m_next_for_endpoint;
    net_http2_stream_t m_stream;
    uint16_t m_id;
    uint8_t m_free_after_processed;
    uint8_t m_on_complete_processed;

    /*req*/
    net_http2_req_method_t m_req_method;
    net_http2_req_state_t m_req_state;
    uint16_t m_req_head_count;
    uint16_t m_req_head_capacity;
    nghttp2_nv * m_req_headers;

    /*res*/
    void * m_res_ctx;
    uint8_t m_res_ignore;
    net_http2_req_on_res_head_fun_t m_res_on_head;
    net_http2_req_on_res_data_fun_t m_res_on_data;
    net_http2_req_on_res_complete_fun_t m_res_on_complete;
    uint16_t m_res_code;
    uint16_t m_res_head_count;
    uint16_t m_res_head_capacity;
    struct net_http2_req_pair * m_res_headers;
};

void net_http2_req_free_i(net_http2_req_t req, uint8_t force);

void net_http2_req_cancel_and_free_i(net_http2_req_t req, uint8_t force);

void net_http2_req_set_stream(net_http2_req_t req, net_http2_stream_t stream);

int net_http2_req_add_res_head(
    net_http2_req_t req,
    const char * attr_name, uint32_t name_len, const char * attr_value, uint32_t value_len);

int net_http2_req_on_req_head_complete(net_http2_req_t req);

NET_END_DECL

#endif
