#ifndef NET_DNS_NS_CLI_ENDPOINT_I_H_INCLEDED
#define NET_DNS_NS_CLI_ENDPOINT_I_H_INCLEDED
#include "net_dns_manage_i.h"
#include "net_dns_ns_parser.h"

NET_BEGIN_DECL

struct net_dns_ns_cli_endpoint {
    net_endpoint_t m_endpoint;
    struct net_dns_ns_parser m_parser;
};

net_dns_ns_cli_endpoint_t net_dns_ns_cli_endpoint_create(net_dns_source_t source, net_driver_t driver);
void net_dns_ns_cli_endpoint_free(net_dns_ns_cli_endpoint_t dns_cli);

net_dns_ns_cli_endpoint_t net_dns_ns_cli_endpoint_get(net_endpoint_t base_endpoint);

int net_dns_ns_cli_endpoint_send(
    net_dns_ns_cli_endpoint_t dns_cli, net_address_t address, void const * data, uint16_t buf_size);
    
int net_dns_ns_cli_endpoint_init(net_endpoint_t base_endpoint);
void net_dns_ns_cli_endpoint_fini(net_endpoint_t base_endpoint);
int net_dns_ns_cli_endpoint_input(net_endpoint_t base_endpoint);
int net_dns_ns_cli_endpoint_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t from_state);

NET_END_DECL

#endif
