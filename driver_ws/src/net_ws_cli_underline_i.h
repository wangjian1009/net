#ifndef NET_WS_CLI_UNDERLINE_I_H_INCLEDED
#define NET_WS_CLI_UNDERLINE_I_H_INCLEDED
#include "net_ws_cli_endpoint_i.h"

struct net_ws_cli_underline_protocol {
    net_ws_cli_driver_t m_driver;
};

enum net_ws_cli_underline_ws_state {
    net_ws_cli_underline_ws_handshake,
    net_ws_cli_underline_ws_open,
};

struct net_ws_cli_underline {
    net_ws_cli_endpoint_t m_ws_endpoint;
    enum net_ws_cli_underline_ws_state m_state;
};

net_protocol_t
net_ws_cli_underline_protocol_create(
    net_schedule_t schedule, const char * name, net_ws_cli_driver_t driver);

int net_ws_cli_underline_write(
    net_endpoint_t base_underline, net_endpoint_t from_ep, net_endpoint_buf_type_t from_buf);

#endif
