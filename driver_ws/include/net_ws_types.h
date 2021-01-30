#ifndef NET_WS_TYPES_H_INCLEDED
#define NET_WS_TYPES_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

typedef struct net_ws_cli_protocol * net_ws_cli_protocol_t;
typedef struct net_ws_cli_endpoint * net_ws_cli_endpoint_t;
typedef struct net_ws_cli_stream_driver * net_ws_cli_stream_driver_t;
typedef struct net_ws_cli_stream_endpoint * net_ws_cli_stream_endpoint_t;

typedef struct net_ws_svr_driver * net_ws_svr_driver_t;
typedef struct net_ws_svr_endpoint * net_ws_svr_endpoint_t;

NET_END_DECL

#endif
