#include "net_protocol.h"
#include "net_dns_svr_protocol_i.h"
#include "net_dns_svr_endpoint_i.h"

static int net_dns_svr_protocol_init(net_protocol_t protocol);
static void net_dns_svr_protocol_fini(net_protocol_t protocol);

net_protocol_t net_dns_svr_protocol_create(net_dns_svr_t dns_svr) {
    net_protocol_t protocol =
        net_protocol_create(
            dns_svr->m_schedule,
            "dns-svr",
            /*protocol*/
            sizeof(struct net_dns_svr_protocol),
            net_dns_svr_protocol_init,
            net_dns_svr_protocol_fini,
            /*endpoint*/
            sizeof(struct net_dns_svr_endpoint),
            net_dns_svr_endpoint_init,
            net_dns_svr_endpoint_fini,
            net_dns_svr_endpoint_input,
            NULL,
            NULL,
            NULL);
    if (protocol == NULL) {
        return NULL;
    }

    net_dns_svr_protocol_t dns_protocol = net_protocol_data(protocol);
    dns_protocol->m_svr = dns_svr;

    return protocol;
}

void net_dns_svr_protocol_free(net_dns_svr_protocol_t dns_protocol) {
    net_protocol_free(net_protocol_from_data(dns_protocol));
}

static int net_dns_svr_protocol_init(net_protocol_t protocol) {
    net_dns_svr_protocol_t dns_protocol = net_protocol_data(protocol);
    dns_protocol->m_svr = NULL;
    return 0;
}

static void net_dns_svr_protocol_fini(net_protocol_t protocol) {
}
