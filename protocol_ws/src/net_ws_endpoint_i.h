#ifndef NET_WS_ENDPOINT_I_H_INCLEDED
#define NET_WS_ENDPOINT_I_H_INCLEDED
#include "wslay/wslay.h"
#include "net_ws_endpoint.h"

NET_BEGIN_DECL

struct net_ws_endpoint {
    net_http_endpoint_t m_http_ep;
    char * m_cfg_path;
    uint32_t m_cfg_pingpong_span_ms;
    net_ws_state_t m_state;
    uint8_t m_ctx_need_reset;
    wslay_event_context_ptr m_ctx;
    char * m_handshake_token;
    uint8_t m_pingpong_count;
    net_timer_t m_pingpong_timer;
};

int net_ws_endpoint_init(net_http_endpoint_t endpoint);
void net_ws_endpoint_fini(net_http_endpoint_t endpoint);
int net_ws_endpoint_input(net_http_endpoint_t endpoint);
int net_ws_endpoint_on_state_change(net_http_endpoint_t endpoint, net_http_state_t from_state);

int net_ws_endpoint_set_state(net_ws_endpoint_t ws_ep, net_ws_state_t state);
    
NET_END_DECL

#endif
