#ifndef NET_WS_CLI_STREAM_ENDPOINT_I_H_INCLEDED
#define NET_WS_CLI_STREAM_ENDPOINT_I_H_INCLEDED
#include "net_ws_cli_stream_endpoint.h"
#include "net_ws_cli_stream_driver_i.h"

struct net_ws_cli_stream_endpoint {
    net_endpoint_t m_underline;
    char * m_path;
};

int net_ws_cli_stream_endpoint_init(net_endpoint_t endpoint);
void net_ws_cli_stream_endpoint_fini(net_endpoint_t endpoint);
int net_ws_cli_stream_endpoint_connect(net_endpoint_t endpoint);
int net_ws_cli_stream_endpoint_update(net_endpoint_t endpoint);
int net_ws_cli_stream_endpoint_set_no_delay(net_endpoint_t endpoint, uint8_t no_delay);
int net_ws_cli_stream_endpoint_get_mss(net_endpoint_t endpoint, uint32_t * mss);

#endif
