#ifndef NET_WS_TYPES_H_INCLEDED
#define NET_WS_TYPES_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

typedef enum net_ws_endpoint_runing_mode net_ws_endpoint_runing_mode_t;
typedef enum net_ws_endpoint_state net_ws_endpoint_state_t;

typedef struct net_ws_protocol * net_ws_protocol_t;
typedef struct net_ws_endpoint * net_ws_endpoint_t;
typedef struct net_ws_stream_driver * net_ws_stream_driver_t;
typedef struct net_ws_stream_endpoint * net_ws_stream_endpoint_t;
typedef struct net_ws_stream_acceptor * net_ws_stream_acceptor_t;

NET_END_DECL

#endif
