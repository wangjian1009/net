#ifndef NET_HTTP_REQ_I_H_INCLEDED
#define NET_HTTP_REQ_I_H_INCLEDED
#include "net_http_req.h"
#include "net_http_endpoint_i.h"

NET_BEGIN_DECL

struct net_http_req {
    net_http_endpoint_t m_http_ep;
    TAILQ_ENTRY(net_http_req) m_next;
    uint16_t m_id;
    net_http_req_state_t m_state;
    uint32_t m_req_size;
    uint32_t m_req_sended_size;
};

void net_http_req_real_free(net_http_req_t req);
int net_http_req_set_state(net_http_endpoint_t http_ep, net_http_state_t state);
    
NET_END_DECL

#endif
