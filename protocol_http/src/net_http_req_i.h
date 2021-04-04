#ifndef NET_HTTP_REQ_I_H_INCLEDED
#define NET_HTTP_REQ_I_H_INCLEDED
#include "net_http_req.h"
#include "net_http_endpoint_i.h"

NET_BEGIN_DECL

enum net_http_req_head_tag {
    net_http_req_head_connection,
    net_http_req_head_content_length,
    net_http_req_head_transfer_encoding,
};

#define net_http_req_head_tag_set(__req, __flag) ((__req)->m_head_tags &= (0x01 << (__flag)))
#define net_http_req_head_tag_is_set(__req, __flag) (((__req)->m_head_tags & (0x01 << (__flag))) ? 1 : 0)

struct net_http_req {
    net_http_endpoint_t m_http_ep;
    TAILQ_ENTRY(net_http_req) m_next;
    uint16_t m_id;
    uint8_t m_is_free;
    uint8_t m_on_complete_processed;

    /*req*/
    net_http_req_method_t m_req_method;
    net_http_req_state_t m_req_state;
    net_http_transfer_encoding_t m_req_transfer_encoding;
    uint32_t m_head_tags;
    uint32_t m_head_size;
    uint32_t m_body_size;
    uint32_t m_body_supply_size;
    uint32_t m_flushed_size;
    
    /*res*/
    void * m_res_ctx;
    uint8_t m_res_completed;
    net_http_req_on_res_begin_fun_t m_res_on_begin;
    net_http_req_on_res_head_fun_t m_res_on_head;
    net_http_req_on_res_body_fun_t m_res_on_body;
    net_http_req_on_res_complete_fun_t m_res_on_complete;
    uint16_t m_res_code;
    char m_res_message[16];
};

int net_http_req_do_send_first_line(
    net_http_protocol_t http_protocol, net_http_req_t http_req, net_http_req_method_t method, const char * url);

void net_http_req_free_force(net_http_req_t req);
void net_http_req_real_free(net_http_req_t req);

NET_END_DECL

#endif
