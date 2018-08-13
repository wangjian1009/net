#include "net_dns_svr_query_i.h"

net_dns_svr_query_t net_dns_svr_query_create(net_dns_svr_itf_t dns_itf) {
    net_dns_svr_t dns_svr = dns_itf->m_svr;
    
    net_dns_svr_query_t query = TAILQ_FIRST(&dns_svr->m_free_querys);
    if (query) {
        TAILQ_REMOVE(&dns_svr->m_free_querys, query, m_next);
    }
    else {
        query = mem_alloc(dns_svr->m_alloc, sizeof(struct net_dns_svr_query));
        if (query == NULL) {
            CPE_ERROR(dns_svr->m_em, "net: dns: query alloc fail!");
            return NULL;
        }
    }

    query->m_itf = dns_itf;

    TAILQ_INSERT_TAIL(&dns_itf->m_querys, query, m_next);
    return query;
}

void net_dns_svr_query_free(net_dns_svr_query_t query) {
    net_dns_svr_t dns_svr = query->m_itf->m_svr;

    TAILQ_REMOVE(&query->m_itf->m_querys, query, m_next);

    query->m_itf = (net_dns_svr_itf_t)dns_svr;
    TAILQ_INSERT_TAIL(&dns_svr->m_free_querys, query, m_next);
}

void net_dns_svr_query_real_free(net_dns_svr_query_t query) {
    net_dns_svr_t dns_svr = (net_dns_svr_t)query->m_itf;

    TAILQ_REMOVE(&dns_svr->m_free_querys, query, m_next);
    mem_free(dns_svr->m_alloc, query);
}
