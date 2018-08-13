#include "net_protocol.h"
#include "net_schedule.h"
#include "net_dns_svr_i.h"
#include "net_dns_svr_itf_i.h"
#include "net_dns_svr_query_i.h"
#include "net_dns_svr_protocol_i.h"

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

    dns_svr->m_dns_protocol =  net_dns_svr_protocol_create(dns_svr);
    if (dns_svr->m_dns_protocol == NULL) {
        CPE_ERROR(em, "net: dns_svr: create dns_svr_protocol fail!");
        mem_free(alloc, dns_svr);
        return NULL;
    }
    
    TAILQ_INIT(&dns_svr->m_itfs);
    TAILQ_INIT(&dns_svr->m_free_querys);
    
    return dns_svr;
}

void net_dns_svr_free(net_dns_svr_t dns_svr) {

    while(!TAILQ_EMPTY(&dns_svr->m_itfs)) {
        net_dns_svr_itf_free(TAILQ_FIRST(&dns_svr->m_itfs));
    }

    net_protocol_free(dns_svr->m_dns_protocol);

    while(!TAILQ_EMPTY(&dns_svr->m_free_querys)) {
        net_dns_svr_query_real_free(TAILQ_FIRST(&dns_svr->m_free_querys));
    }

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

mem_buffer_t net_dns_svr_tmp_buffer(net_dns_svr_t dns_svr) {
    return net_schedule_tmp_buffer(dns_svr->m_schedule);
}
