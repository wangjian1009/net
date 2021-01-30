#ifndef NET_WS_CLI_ENDPOINT_H_INCLEDED
#define NET_WS_CLI_ENDPOINT_H_INCLEDED
#include "net_ws_types.h"

NET_BEGIN_DECL

net_ws_cli_endpoint_t net_ws_cli_endoint_cast(net_endpoint_t endpoint);
net_endpoint_t net_ws_cli_endpoint_stream(net_endpoint_t endpoint);

const char * net_ws_cli_endpoint_path(net_ws_cli_endpoint_t endpoint);
int net_ws_cli_endpoint_set_path(net_ws_cli_endpoint_t endpoint, const char * path);

/* int net_ws_cli_endpoint_send_msg_text(net_ws_endpoint_t ws_ep, const char * msg); */
/* int net_ws_cli_endpoint_send_msg_bin(net_ws_endpoint_t ws_ep, const void * msg, uint32_t msg_len); */

NET_END_DECL

#endif
