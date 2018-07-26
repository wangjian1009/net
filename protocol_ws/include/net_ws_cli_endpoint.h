#ifndef NET_WS_CLI_ENDPOINT_H_INCLEDED
#define NET_WS_CLI_ENDPOINT_H_INCLEDED
#include "net_ws_types.h"

NET_BEGIN_DECL

net_ws_cli_endpoint_t net_ws_cli_endpoint_create(
    net_driver_t driver, net_endpoint_type_t type, net_ws_cli_protocol_t ws_protocol,
    const char * url);
void net_ws_cli_endpoint_free(net_ws_cli_endpoint_t ws_ep);

net_ws_cli_endpoint_t net_ws_cli_endpoint_get(net_endpoint_t endpoint);

void * net_ws_cli_endpoint_data(net_ws_cli_endpoint_t ws_ep);
net_ws_cli_endpoint_t net_ws_cli_endpoint_from_data(void * data);

const char * net_ws_cli_endpoint_url(net_ws_cli_endpoint_t ws_ep);

net_ws_cli_state_t net_ws_cli_endpoint_state(net_ws_cli_endpoint_t ws_ep);

NET_END_DECL

#endif
