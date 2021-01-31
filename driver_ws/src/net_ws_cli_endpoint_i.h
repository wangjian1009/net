#ifndef NET_WS_CLI_ENDPOINT_I_H_INCLEDED
#define NET_WS_CLI_ENDPOINT_I_H_INCLEDED
#include <wslay/wslay.h>
#include "net_ws_cli_endpoint.h"
#include "net_ws_cli_protocol_i.h"

typedef enum net_ws_cli_endpoint_handshake_state {
    net_ws_cli_endpoint_handshake_first_line,
    net_ws_cli_endpoint_handshake_header_line,
} net_ws_cli_endpoint_handshake_state_t;

typedef enum net_ws_cli_endpoint_handshake_field {
    net_ws_cli_endpoint_handshake_field_upgrade,
    net_ws_cli_endpoint_handshake_field_connection,
    net_ws_cli_endpoint_handshake_field_accept,
} net_ws_cli_endpoint_handshake_field_t;

struct net_ws_cli_endpoint {
    net_endpoint_t m_base_endpoint;
    net_ws_cli_stream_endpoint_t m_stream;
    net_ws_cli_endpoint_state_t m_state;
    struct {
        char m_key[16];
        net_ws_cli_endpoint_handshake_state_t m_state;
        uint16_t m_readed_size;
        uint8_t m_received_fields;
    } m_handshake;
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

/*handshake*/
int net_ws_cli_endpoint_send_handshake(net_endpoint_t base_endpoint, net_ws_cli_endpoint_t endpoint);
int net_ws_cli_endpoint_input_handshake(net_endpoint_t base_endpoint, net_ws_cli_endpoint_t endpoint);

#endif
