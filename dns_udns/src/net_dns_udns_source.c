#include "cpe/pal/pal_strings.h"
#include "net_dns_manage.h"
#include "net_dns_source.h"
#include "net_dns_udns_source_i.h"
#include "net_dns_udns_source_ctx_i.h"

int net_dns_udns_source_init(net_dns_source_t source);
void net_dns_udns_source_fini(net_dns_source_t source);
void net_dns_udns_source_dump(write_stream_t ws, net_dns_source_t source);

net_dns_udns_source_t
net_dns_udns_source_create(
    mem_allocrator_t alloc, error_monitor_t em,
    net_dns_manage_t manage, net_driver_t driver)
{
    net_dns_source_t source =
        net_dns_source_create(
            manage,
            sizeof(struct net_dns_udns_source),
            net_dns_udns_source_init,
            net_dns_udns_source_fini,
            net_dns_udns_source_dump,
            sizeof(struct net_dns_udns_source_ctx),
            net_dns_udns_source_ctx_init,
            net_dns_udns_source_ctx_fini,
            net_dns_udns_source_ctx_start,
            net_dns_udns_source_ctx_cancel);
    if (source == NULL) return NULL;

    net_dns_udns_source_t udns = net_dns_source_data(source);

    udns->m_alloc = alloc;
    udns->m_em = em;
    udns->m_manage = manage;
    udns->m_driver = driver;

    udns->m_dns_ctx = dns_new(&dns_defctx);
    if (udns->m_dns_ctx == NULL) {
        CPE_ERROR(em, "udns: dns_new fail!");
        net_dns_source_free(source);
        return NULL;
    }
    
    return udns;
}

void net_dns_udns_source_free(net_dns_udns_source_t udns) {
    net_dns_source_free(net_dns_source_from_data(udns));
}

net_dns_udns_source_t net_dns_udns_source_from_source(net_dns_source_t source) {
    return net_dns_source_init(source) == net_dns_udns_source_init
        ? net_dns_source_data(source)
        : NULL;
}

int net_dns_udns_source_init(net_dns_source_t source) {
    net_dns_udns_source_t udns = net_dns_source_data(source);

    bzero(udns, sizeof(*udns));

    return 0;
}

void net_dns_udns_source_fini(net_dns_source_t source) {
    net_dns_udns_source_t udns = net_dns_source_data(source);

    if (udns->m_dns_ctx) {
        dns_free(udns->m_dns_ctx);
        udns->m_dns_ctx = NULL;
    }
}

void net_dns_udns_source_dump(write_stream_t ws, net_dns_source_t source) {
}
