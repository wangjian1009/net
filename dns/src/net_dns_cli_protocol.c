#include "net_protocol.h"
#include "net_dns_cli_protocol_i.h"
#include "net_dns_cli_endpoint_i.h"

static int net_dns_cli_protocol_init(net_protocol_t protocol);
static void net_dns_cli_protocol_fini(net_protocol_t protocol);

net_dns_cli_protocol_t net_dns_cli_protocol_create(net_dns_manage_t manage) {
    net_protocol_t protocol =
        net_protocol_create(
            manage->m_schedule,
            "dns-cli",
            /*protocol*/
            sizeof(struct net_dns_cli_protocol),
            net_dns_cli_protocol_init,
            net_dns_cli_protocol_fini,
            /*endpoint*/
            sizeof(struct net_dns_cli_endpoint),
            net_dns_cli_endpoint_init,
            net_dns_cli_endpoint_fini,
            net_dns_cli_endpoint_input,
            NULL,
            NULL,
            NULL);
    if (protocol == NULL) {
        return NULL;
    }

    net_dns_cli_protocol_t dns_protocol = net_protocol_data(protocol);
    dns_protocol->m_manage = manage;

    return dns_protocol;
}

void net_dns_cli_protocol_free(net_dns_cli_protocol_t dns_protocol) {
    net_protocol_free(net_protocol_from_data(dns_protocol));
}

static int net_dns_cli_protocol_init(net_protocol_t protocol) {
    net_dns_cli_protocol_t dns_protocol = net_protocol_data(protocol);
    dns_protocol->m_manage = NULL;
    return 0;
}

static void net_dns_cli_protocol_fini(net_protocol_t protocol) {
}
