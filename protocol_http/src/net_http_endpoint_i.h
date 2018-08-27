#ifndef NET_HTTP_ENDPOINT_I_H_INCLEDED
#define NET_HTTP_ENDPOINT_I_H_INCLEDED
#include "net_http_endpoint.h"

NET_BEGIN_DECL

struct net_http_endpoint {
    net_endpoint_t m_endpoint;
    uint8_t m_cfg_use_https;
    uint32_t m_cfg_reconnect_span_ms;
    net_http_state_t m_state;
    net_timer_t m_connect_timer;
    net_timer_t m_process_timer;
};

int net_http_endpoint_init(net_endpoint_t endpoint);
void net_http_endpoint_fini(net_endpoint_t endpoint);
int net_http_endpoint_input(net_endpoint_t endpoint);
int net_http_endpoint_on_state_change(net_endpoint_t endpoint, net_endpoint_state_t from_state);

int net_http_endpoint_set_state(net_http_endpoint_t http_ep, net_http_state_t state);
    
NET_END_DECL

#endif
