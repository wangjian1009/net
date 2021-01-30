#ifndef NET_WS_CLI_ENDPOINT_I_H_INCLEDED
#define NET_WS_CLI_ENDPOINT_I_H_INCLEDED
#include <wslay/wslay.h>
#include "net_ws_cli_endpoint.h"
#include "net_ws_cli_protocol_i.h"

enum net_ws_cli_handshake_state {
    net_ws_cli_handshake_processing,
    net_ws_cli_handshake_done,
};

struct net_ws_cli_endpoint {
    net_ws_cli_stream_endpoint_t m_stream;
    enum net_ws_cli_handshake_state m_handshake_state;
    wslay_event_context_ptr m_ctx;
    char * m_path;
};

int net_ws_cli_endpoint_write(
    net_endpoint_t base_endpoint, net_endpoint_t from_ep, net_endpoint_buf_type_t from_buf);

/**/
int net_ws_cli_endpoint_init(net_endpoint_t base_endpoint);
void net_ws_cli_endpoint_fini(net_endpoint_t base_endpoint);
int net_ws_cli_endpoint_input(net_endpoint_t base_endpoint);
int net_ws_cli_endpoint_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t from_state);

#endif
