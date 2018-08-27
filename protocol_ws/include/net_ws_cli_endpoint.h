#ifndef NET_WS_CLI_ENDPOINT_H_INCLEDED
#define NET_WS_CLI_ENDPOINT_H_INCLEDED
#include "net_http_types.h"
#include "net_ws_types.h"

NET_BEGIN_DECL

net_ws_cli_endpoint_t net_ws_cli_endpoint_create(
    net_driver_t driver, net_endpoint_type_t type, net_ws_cli_protocol_t ws_protocol);
void net_ws_cli_endpoint_free(net_ws_cli_endpoint_t ws_ep);

net_ws_cli_endpoint_t net_ws_cli_endpoint_get(net_http_endpoint_t endpoint);

int net_ws_cli_endpoint_set_remote_and_path(net_ws_cli_endpoint_t ws_ep, const char * url);

void net_ws_cli_endpoint_url_print(write_stream_t s, net_ws_cli_endpoint_t ws_ep);
const char * net_ws_cli_endpoint_url_dump(mem_buffer_t buffer, net_ws_cli_endpoint_t ws_ep);

const char * net_ws_cli_endpoint_path(net_ws_cli_endpoint_t ws_ep);
int net_ws_cli_endpoint_set_path(net_ws_cli_endpoint_t ws_ep, const char * path);

uint32_t net_ws_cli_endpoint_reconnect_span_ms(net_ws_cli_endpoint_t ws_ep);
void net_ws_cli_endpoint_set_reconnect_span_ms(net_ws_cli_endpoint_t ws_ep, uint32_t span_ms);

void * net_ws_cli_endpoint_data(net_ws_cli_endpoint_t ws_ep);
net_ws_cli_endpoint_t net_ws_cli_endpoint_from_data(void * data);

net_ws_cli_state_t net_ws_cli_endpoint_state(net_ws_cli_endpoint_t ws_ep);

void net_ws_cli_endpoint_enable(net_ws_cli_endpoint_t ws_ep);

net_endpoint_t net_ws_cli_endpoint_net_ep(net_ws_cli_endpoint_t ws_ep);
net_http_endpoint_t net_ws_cli_endpoint_http_ep(net_ws_cli_endpoint_t ws_ep);

int net_ws_cli_endpoint_send_msg_text(net_ws_cli_endpoint_t ws_ep, const char * msg);
int net_ws_cli_endpoint_send_msg_bin(net_ws_cli_endpoint_t ws_ep, const void * msg, uint32_t msg_len);

const char * net_ws_cli_state_str(net_ws_cli_state_t state);

NET_END_DECL

#endif
