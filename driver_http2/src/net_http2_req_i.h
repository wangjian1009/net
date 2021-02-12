#ifndef NET_HTTP2_REQ_I_H_INCLEDED
#define NET_HTTP2_REQ_I_H_INCLEDED
#include "net_http2_req.h"
#include "net_http2_endpoint_i.h"

NET_BEGIN_DECL

struct net_http2_req {
    net_http2_endpoint_t m_endpoint;
    TAILQ_ENTRY(net_http2_req) m_next;
    uint16_t m_id;
    uint8_t m_data_sended;
    uint8_t m_free_after_processed;
    uint8_t m_on_complete_processed;

    /*req*/
    net_http2_req_method_t m_req_method;
    net_http2_req_state_t m_req_state;
    
    /*res*/
    void * m_res_ctx;
    uint8_t m_res_ignore;
    net_http2_req_on_res_begin_fun_t m_res_on_begin;
    net_http2_req_on_res_head_fun_t m_res_on_head;
    net_http2_req_on_res_body_fun_t m_res_on_body;
    net_http2_req_on_res_complete_fun_t m_res_on_complete;
    uint16_t m_res_code;
    char m_res_message[16];
};

void net_http2_req_free_i(net_http2_req_t req, uint8_t force);

void net_http2_req_cancel_and_free_i(net_http2_req_t req, uint8_t force);

NET_END_DECL

#endif
