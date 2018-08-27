#ifndef NET_WS_CLI_ENDPOINT_I_H_INCLEDED
#define NET_WS_CLI_ENDPOINT_I_H_INCLEDED
#include "wslay/wslay.h"
#include "net_ws_cli_endpoint.h"

NET_BEGIN_DECL

struct net_ws_cli_endpoint {
    net_http_endpoint_t m_http_ep;
    char * m_cfg_path;
    net_ws_cli_state_t m_state;
    net_timer_t m_process_timer;
    wslay_event_context_ptr m_ctx;
    char * m_handshake_token;
};

int net_ws_cli_endpoint_init(net_http_endpoint_t endpoint);
void net_ws_cli_endpoint_fini(net_http_endpoint_t endpoint);
int net_ws_cli_endpoint_input(net_http_endpoint_t endpoint);
int net_ws_cli_endpoint_on_state_change(net_http_endpoint_t endpoint, net_http_state_t from_state);

int net_ws_cli_endpoint_set_state(net_ws_cli_endpoint_t ws_ep, net_ws_cli_state_t state);
    
NET_END_DECL

#endif
