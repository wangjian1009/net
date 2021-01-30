#ifndef NET_WS_CLI_ENDPOINT_H_INCLEDED
#define NET_WS_CLI_ENDPOINT_H_INCLEDED
#include "net_ws_types.h"

NET_BEGIN_DECL

net_ws_cli_endpoint_t net_ws_cli_endoint_cast(net_endpoint_t endpoint);

int net_ws_cli_endpoint_set_path(net_ws_cli_endpoint_t ws_endpoint);
    
NET_END_DECL

#endif
