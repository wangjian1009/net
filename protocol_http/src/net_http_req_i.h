#ifndef NET_HTTP_REQ_I_H_INCLEDED
#define NET_HTTP_REQ_I_H_INCLEDED
#include "net_http_req.h"
#include "net_http_endpoint_i.h"

NET_BEGIN_DECL

struct net_http_req {
    net_http_endpoint_t m_http_ep;
    TAILQ_ENTRY(net_http_req) m_next;
    uint16_t m_id;

    /*req*/
    net_http_req_state_t m_req_state;
    uint32_t m_req_size;
    uint32_t m_body_size;

    /*res*/
    net_http_res_state_t m_res_state;
    void * m_res_ctx;
    net_http_req_on_res_begin_fun_t m_res_on_begin;
    net_http_req_on_res_head_fun_t m_res_on_head;
    net_http_req_on_res_body_fun_t m_res_on_body;
    net_http_req_on_res_complete_fun_t m_res_on_complete;
    uint32_t m_res_received_size;
};

int net_http_req_do_send_first_line(
    net_http_protocol_t http_protocol, net_http_req_t http_req, net_http_req_method_t method, const char * url);

int net_http_endpoint_req_input(net_http_protocol_t ws_protocol, net_http_req_t req);

void net_http_req_real_free(net_http_req_t req);
int net_http_req_set_state(net_http_endpoint_t http_ep, net_http_state_t state);
    
NET_END_DECL

#endif
