#ifndef NET_DNS_NS_CLI_ENDPOINT_I_H_INCLEDED
#define NET_DNS_NS_CLI_ENDPOINT_I_H_INCLEDED
#include "net_dns_manage_i.h"

NET_BEGIN_DECL

struct net_dns_ns_cli_endpoint {
    int dummy;
};

int net_dns_ns_cli_endpoint_init(net_endpoint_t base_endpoint);
void net_dns_ns_cli_endpoint_fini(net_endpoint_t base_endpoint);
int net_dns_ns_cli_endpoint_input(net_endpoint_t base_endpoint);

net_dns_ns_cli_endpoint_t net_dns_ns_cli_endpoint_get(net_endpoint_t base_endpoint);

NET_END_DECL

#endif
