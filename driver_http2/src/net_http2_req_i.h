#ifndef NET_HTTP2_REQ_I_H_INCLEDED
#define NET_HTTP2_REQ_I_H_INCLEDED
#include "net_http2_req.h"
#include "net_http2_endpoint_i.h"

NET_BEGIN_DECL

struct net_http2_req {
    net_http2_endpoint_t m_endpoint;
    TAILQ_ENTRY(net_http2_req) m_next_for_endpoint;
    net_http2_stream_t m_stream;
    uint16_t m_id;
    net_http2_req_state_t m_state;

    /*req*/
    uint16_t m_req_head_count;
    uint16_t m_req_head_capacity;
    nghttp2_nv * m_req_headers;

    /*res*/
    uint16_t m_res_head_count;
    uint16_t m_res_head_capacity;
    nghttp2_nv * m_res_headers;

    /*read callback*/
    void * m_read_ctx;
    net_http2_req_on_state_change_fun_t m_on_state_change;
    net_http2_req_on_recv_fun_t m_on_recv;
    void (*m_read_ctx_free)(void *);

    /*write callback*/
    void * m_write_ctx;
    net_http2_req_on_write_fun_t m_on_write;
    void (*m_write_ctx_free)(void *);
    uint8_t m_have_follow_data;
    uint8_t m_is_write_started;
    struct mem_buffer m_write_data_buffer;
};

int net_http2_req_begin_write(net_http2_req_t req);

void net_http2_req_set_req_state(net_http2_req_t req, net_http2_req_state_t state);

void net_http2_req_set_stream(net_http2_req_t req, net_http2_stream_t stream);

int net_http2_req_add_req_head2(
    net_http2_req_t req,
    const char * attr_name, uint32_t name_len, const char * attr_value, uint32_t value_len);
    
int net_http2_req_add_res_head2(
    net_http2_req_t req,
    const char * attr_name, uint32_t name_len, const char * attr_value, uint32_t value_len);

int net_http2_req_on_req_head_complete(net_http2_req_t req);

NET_END_DECL

#endif
