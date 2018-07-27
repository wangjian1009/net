#ifndef NET_WS_CLI_TYPES_H_INCLEDED
#define NET_WS_CLI_TYPES_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef struct net_ws_cli_protocol * net_ws_cli_protocol_t;
typedef struct net_ws_cli_endpoint * net_ws_cli_endpoint_t;

typedef enum net_ws_cli_state {
    net_ws_cli_state_init,
    net_ws_cli_state_connecting,
    net_ws_cli_state_handshake,
    net_ws_cli_state_established,
    net_ws_cli_state_error,
} net_ws_cli_state_t;

NET_END_DECL

#endif
