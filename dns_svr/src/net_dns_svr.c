#include "net_dns_svr_i.h"

net_dns_svr_t net_dns_svr_create(mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule) {
    net_dns_svr_t dns_svr = mem_calloc(alloc, sizeof(struct net_dns_svr));
    if (dns_svr == NULL) {
        CPE_ERROR(em, "net: dns_svr: alloc dns_svr fail!");
        return NULL;
    }

    dns_svr->m_alloc = alloc;
    dns_svr->m_em = em;
    dns_svr->m_debug = 0;
    dns_svr->m_schedule = schedule;

    return dns_svr;
}

void net_dns_svr_free(net_dns_svr_t dns_svr) {
    mem_free(dns_svr->m_alloc, dns_svr);
}

uint8_t net_dns_svr_debug(net_dns_svr_t dns_svr) {
    return dns_svr->m_debug;
}

void net_dns_svr_set_debug(net_dns_svr_t dns_svr, uint8_t debug) {
    dns_svr->m_debug = debug;
}

net_schedule_t net_dns_svr_schedule(net_dns_svr_t dns_svr) {
    return dns_svr->m_schedule;
}

