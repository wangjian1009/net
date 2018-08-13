#include "cpe/utils/string_utils.h"
#include "net_dns_svr_query_entry_i.h"

net_dns_svr_query_entry_t net_dns_svr_query_entry_create(net_dns_svr_query_t query, const char * domain_name) {
    net_dns_svr_t dns_svr = query->m_itf->m_svr;
    
    net_dns_svr_query_entry_t query_entry = TAILQ_FIRST(&dns_svr->m_free_query_entries);
    if (query_entry) {
        TAILQ_REMOVE(&dns_svr->m_free_query_entries, query_entry, m_next);
    }
    else {
        query_entry = mem_alloc(dns_svr->m_alloc, sizeof(struct net_dns_svr_query_entry));
        if (query_entry == NULL) {
            CPE_ERROR(dns_svr->m_em, "net: dns: query_entry alloc fail!");
            return NULL;
        }
    }

    query_entry->m_query = query;
    cpe_str_dup(query_entry->m_domain_name, sizeof(query_entry->m_domain_name), domain_name);
    
    TAILQ_INSERT_TAIL(&query->m_entries, query_entry, m_next);

    return query_entry;
}

void net_dns_svr_query_entry_free(net_dns_svr_query_entry_t query_entry) {
    net_dns_svr_t dns_svr = query_entry->m_query->m_itf->m_svr;

    TAILQ_REMOVE(&query_entry->m_query->m_entries, query_entry, m_next);

    query_entry->m_query = (net_dns_svr_query_t)dns_svr;
    TAILQ_INSERT_TAIL(&dns_svr->m_free_query_entries, query_entry, m_next);
}

void net_dns_svr_query_entry_real_free(net_dns_svr_query_entry_t query_entry) {
    net_dns_svr_t dns_svr = (net_dns_svr_t)query_entry->m_query;

    TAILQ_REMOVE(&dns_svr->m_free_query_entries, query_entry, m_next);
    mem_free(dns_svr->m_alloc, query_entry);
}


