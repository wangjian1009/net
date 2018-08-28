#ifndef NET_WS_TYPES_H_INCLEDED
#define NET_WS_TYPES_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef struct net_ws_protocol * net_ws_protocol_t;
typedef struct net_ws_endpoint * net_ws_endpoint_t;

typedef enum net_ws_state {
    net_ws_state_init,
    net_ws_state_connecting,
    net_ws_state_handshake,
    net_ws_state_established,
    net_ws_state_error,
} net_ws_state_t;

NET_END_DECL

#endif
