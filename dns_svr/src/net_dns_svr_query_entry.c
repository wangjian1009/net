#include <assert.h>
#include "cpe/utils/string_utils.h"
#include "net_dns_query.h"
#include "net_address.h"
#include "net_dns_svr_query_entry_i.h"

static void net_dns_svr_query_entry_callback(void * ctx, net_address_t main_address, net_address_it_t all_address);

net_dns_svr_query_entry_t net_dns_svr_query_entry_create(net_dns_svr_query_t query, const char * domain_name) {
    net_dns_svr_t dns_svr = query->m_itf->m_svr;
    
    net_dns_svr_query_entry_t query_entry = TAILQ_FIRST(&dns_svr->m_free_query_entries);
    if (query_entry) {
        TAILQ_REMOVE(&dns_svr->m_free_query_entries, query_entry, m_next);
    }
    else {
        query_entry = mem_alloc(dns_svr->m_alloc, sizeof(struct net_dns_svr_query_entry));
        if (query_entry == NULL) {
            CPE_ERROR(dns_svr->m_em, "dns-svr: query_entry alloc fail!");
            return NULL;
        }
    }

    query_entry->m_query = query;
    cpe_str_dup(query_entry->m_domain_name, sizeof(query_entry->m_domain_name), domain_name);
    query_entry->m_result = NULL;
    query_entry->m_local_query = NULL;

    TAILQ_INSERT_TAIL(&query->m_entries, query_entry, m_next);

    return query_entry;
}

void net_dns_svr_query_entry_free(net_dns_svr_query_entry_t query_entry) {
    net_dns_svr_t dns_svr = query_entry->m_query->m_itf->m_svr;

    if (query_entry->m_local_query) {
        net_dns_query_free(query_entry->m_local_query);
        query_entry->m_local_query = NULL;
        assert(query_entry->m_query->m_runing_entry_count > 0);
        query_entry->m_query->m_runing_entry_count--;
    }

    if (query_entry->m_result) {
        net_address_free(query_entry->m_result);
        query_entry->m_result = NULL;
    }
    
    TAILQ_REMOVE(&query_entry->m_query->m_entries, query_entry, m_next);

    query_entry->m_query = (net_dns_svr_query_t)dns_svr;
    TAILQ_INSERT_TAIL(&dns_svr->m_free_query_entries, query_entry, m_next);
}

void net_dns_svr_query_entry_real_free(net_dns_svr_query_entry_t query_entry) {
    net_dns_svr_t dns_svr = (net_dns_svr_t)query_entry->m_query;

    TAILQ_REMOVE(&dns_svr->m_free_query_entries, query_entry, m_next);
    mem_free(dns_svr->m_alloc, query_entry);
}

int net_dns_svr_query_entry_start(net_dns_svr_query_entry_t query_entry) {
    net_dns_svr_t svr = query_entry->m_query->m_itf->m_svr;
    
    assert(query_entry->m_local_query == NULL);
           
    /*启动查询 */
    query_entry->m_local_query =
        net_dns_query_create(
            svr->m_schedule,
            query_entry->m_domain_name,
            net_dns_svr_query_entry_callback, NULL, query_entry);
    if (query_entry->m_local_query == NULL) {
        CPE_ERROR(svr->m_em, "dns-svr: start local query fail!");
        return -1;
    }
    query_entry->m_query->m_runing_entry_count++;

    return 0;
}

static void net_dns_svr_query_entry_callback(void * ctx, net_address_t main_address, net_address_it_t all_address) {
    net_dns_svr_query_entry_t query_entry = ctx;
    net_dns_svr_query_t query = query_entry->m_query;
    net_dns_svr_itf_t itf = query->m_itf;
    net_dns_svr_t svr = itf->m_svr;
    
    assert(query_entry->m_local_query);
    query_entry->m_local_query = NULL;
    assert(query_entry->m_query->m_runing_entry_count > 0);
    query_entry->m_query->m_runing_entry_count--;

    if (svr->m_debug) {
        CPE_ERROR(
            svr->m_em, "dns-svr: query %s ==> %s",
            query_entry->m_domain_name,
            main_address ? net_address_dump(net_dns_svr_tmp_buffer(svr), main_address) : "none");
    }
    
    if (main_address) {
        query_entry->m_result = net_address_copy(svr->m_schedule, main_address);
        if (query_entry->m_result == NULL) {
            CPE_ERROR(svr->m_em, "dns-svr: query %s: address copy fail", query_entry->m_domain_name);
        }
    }

    if (query_entry->m_query->m_runing_entry_count == 0) {
        net_dns_svr_itf_send_response(itf, query);
        net_dns_svr_query_free(query);
    }
}

