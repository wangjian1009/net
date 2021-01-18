#ifndef NET_SSL_CLI_ENDPOINT_I_H_INCLEDED
#define NET_SSL_CLI_ENDPOINT_I_H_INCLEDED
#include "net_ssl_cli_endpoint.h"
#include "net_ssl_cli_driver_i.h"

struct net_ssl_cli_endpoint {
    net_endpoint_t m_underline;
};

int net_ssl_cli_endpoint_init(net_endpoint_t endpoint);
void net_ssl_cli_endpoint_fini(net_endpoint_t endpoint);
int net_ssl_cli_endpoint_connect(net_endpoint_t endpoint);
void net_ssl_cli_endpoint_close(net_endpoint_t endpoint);
int net_ssl_cli_endpoint_update(net_endpoint_t endpoint);
int net_ssl_cli_endpoint_set_no_delay(net_endpoint_t endpoint, uint8_t no_delay);
int net_ssl_cli_endpoint_get_mss(net_endpoint_t endpoint, uint32_t * mss);

#endif
