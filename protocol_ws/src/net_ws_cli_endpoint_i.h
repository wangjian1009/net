#ifndef NET_WS_CLI_ENDPOINT_I_H_INCLEDED
#define NET_WS_CLI_ENDPOINT_I_H_INCLEDED
#include "wslay/wslay.h"
#include "net_ws_cli_endpoint.h"

NET_BEGIN_DECL

struct net_ws_cli_endpoint {
    net_endpoint_t m_endpoint;
    net_ws_cli_state_t m_state;
    wslay_event_context_ptr m_ctx;
};

int net_ws_cli_endpoint_init(net_endpoint_t endpoint);
void net_ws_cli_endpoint_fini(net_endpoint_t endpoint);
int net_ws_cli_endpoint_input(net_endpoint_t endpoint);
int net_ws_cli_endpoint_on_state_change(net_endpoint_t endpoint, net_endpoint_state_t from_state);

NET_END_DECL

#endif
