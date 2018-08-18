#include "net_endpoint.h"
#include "net_dns_cli_endpoint_i.h"
#include "net_dns_cli_protocol_i.h"

net_dns_cli_endpoint_t net_dns_cli_endpoint_get(net_endpoint_t base_endpoint) {
    return net_endpoint_data(base_endpoint);
}

int net_dns_cli_endpoint_init(net_endpoint_t base_endpoint) {
    //net_dns_cli_endpoint_t dns_ep = net_endpoint_protocol_data(base_endpoint);
    /* net_dns_cli_protocol_t dns_protocol = net_protocol_data(net_endpoint_protocol(endpoint)); */
    //dns_ep->m_itf = NULL;
    return 0;
}

void net_dns_cli_endpoint_fini(net_endpoint_t base_endpoint) {
}

int net_dns_cli_endpoint_input(net_endpoint_t base_endpoint) {
    /* net_dns_cli_endpoint_t dns_ep = net_endpoint_protocol_data(endpoint); */
    return 0;
}

