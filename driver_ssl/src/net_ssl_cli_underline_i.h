#ifndef NET_SSL_CLI_UNDERLINE_I_H_INCLEDED
#define NET_SSL_CLI_UNDERLINE_I_H_INCLEDED
#include "net_ssl_cli_endpoint_i.h"

struct net_ssl_cli_underline_protocol {
    net_ssl_cli_driver_t m_driver;
};

enum net_ssl_cli_underline_ssl_state {
    net_ssl_cli_underline_ssl_handshake,
    net_ssl_cli_underline_ssl_open,
};

struct net_ssl_cli_underline {
    net_ssl_cli_endpoint_t m_ssl_endpoint;
    enum net_ssl_cli_underline_ssl_state m_state;
	SSL * m_ssl;
};

net_protocol_t
net_ssl_cli_underline_protocol_create(
    net_schedule_t schedule, const char * name, net_ssl_cli_driver_t driver);

int net_ssl_cli_underline_write(
    net_endpoint_t base_underline, net_endpoint_t from_ep, net_endpoint_buf_type_t from_buf);

#endif