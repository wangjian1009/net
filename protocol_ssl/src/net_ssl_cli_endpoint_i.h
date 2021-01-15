#ifndef NET_SSL_CLI_ENDPOINT_I_H_INCLEDED
#define NET_SSL_CLI_ENDPOINT_I_H_INCLEDED
#include "net_ssl_cli_protocol.h"

struct net_ssl_cli_endpoint {
    
};

int net_ssl_cli_endpoint_init(net_endpoint_t endpoint);
void net_ssl_cli_endpoint_fini(net_endpoint_t endpoint);
int net_ssl_cli_endpoint_input(net_endpoint_t endpoint);
int net_ssl_cli_endpoint_on_state_change(net_endpoint_t endpoint, net_endpoint_state_t from_state);

#endif
