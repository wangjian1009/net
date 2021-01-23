#ifndef NET_HTTP_REQ_I_H_INCLEDED
#define NET_HTTP_REQ_I_H_INCLEDED
#include "net_http_req.h"
#include "net_http_endpoint_i.h"

NET_BEGIN_DECL

struct net_http_req {
    net_http_endpoint_t m_http_ep;
    TAILQ_ENTRY(net_http_req) m_next;
    uint16_t m_id;
    uint8_t m_data_sended;
    uint8_t m_free_after_processed;

    /*req*/
    net_http_req_method_t m_req_method;
    net_http_req_state_t m_req_state;
    uint8_t m_req_have_keep_alive;
    uint32_t m_head_size;
    uint32_t m_body_size;
    uint32_t m_flushed_size;
    
    /*res*/
    void * m_res_ctx;
    uint8_t m_res_ignore;
    net_http_req_on_res_begin_fun_t m_res_on_begin;
    net_http_req_on_res_head_fun_t m_res_on_head;
    net_http_req_on_res_body_fun_t m_res_on_body;
    net_http_req_on_res_complete_fun_t m_res_on_complete;
    uint16_t m_res_code;
    char m_res_message[16];
};

int net_http_req_do_send_first_line(
    net_http_protocol_t http_protocol, net_http_req_t http_req, net_http_req_method_t method, const char * url);

void net_http_req_free_i(net_http_req_t req, uint8_t force);
void net_http_req_real_free(net_http_req_t req);

void net_http_req_cancel_and_free_i(net_http_req_t req, uint8_t force);

NET_END_DECL

#endif
