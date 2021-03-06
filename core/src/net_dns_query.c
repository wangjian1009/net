#include <assert.h>
#include "net_address.h"
#include "net_dns_query_i.h"

net_dns_query_t net_dns_query_create(
    net_schedule_t schedule,
    net_address_t hostname,
    net_dns_query_type_t query_type,
    const char * policy,
    net_dns_query_callback_fun_t callback,
    net_dns_query_ctx_free_fun_t ctx_free,
    void * ctx)
{
    if (schedule->m_dns_resolver_ctx == NULL) {
        CPE_ERROR(
            schedule->m_em, "dns-query: %s: no dns resolver!",
            net_address_host(net_schedule_tmp_buffer(schedule), hostname));
        return NULL;
    }
    
    net_dns_query_t query = TAILQ_FIRST(&schedule->m_free_dns_querys);
    if (query) {
        TAILQ_REMOVE(&schedule->m_free_dns_querys, query, m_next);
    }
    else {
        query = mem_alloc(schedule->m_alloc, sizeof(struct net_dns_query) + schedule->m_dns_query_capacity);
        if (query == NULL) {
            CPE_ERROR(
                schedule->m_em, "dns-query: %s: alloc fail!",
                net_address_host(net_schedule_tmp_buffer(schedule), hostname));
            return NULL;
        }
    }

    query->m_schedule = schedule;
    query->m_query_id = ++schedule->m_dns_max_query_id;
    query->m_callback = callback;
    query->m_ctx_free = ctx_free;
    query->m_ctx = ctx;
    query->m_processing = 0;
    
    cpe_hash_entry_init(&query->m_hh);
    if (cpe_hash_table_insert_unique(&schedule->m_dns_querys, query) != 0) {
        CPE_ERROR(
            schedule->m_em, "dns-query: %s: id duplicate!",
            net_address_host(net_schedule_tmp_buffer(schedule), hostname));
        TAILQ_INSERT_TAIL(&schedule->m_free_dns_querys, query, m_next);
        return NULL;
    }

    query->m_processing = 1;
    if (schedule->m_dns_query_init_fun(schedule->m_dns_resolver_ctx, query, hostname, query_type, policy) != 0) {
        CPE_ERROR(
            schedule->m_em, "dns-query: %s: init fail!",
            net_address_host(net_schedule_tmp_buffer(schedule), hostname));
        cpe_hash_table_remove_by_ins(&schedule->m_dns_querys, query);
        TAILQ_INSERT_TAIL(&schedule->m_free_dns_querys, query, m_next);
        return NULL;
    }
    query->m_processing = 0;
    
    return query;
}

void net_dns_query_free(net_dns_query_t query) {
    net_schedule_t schedule = query->m_schedule;

    schedule->m_dns_query_fini_fun(schedule->m_dns_resolver_ctx, query);
    
    if (query->m_ctx_free) {
        query->m_ctx_free(query->m_ctx);
    }
    
    cpe_hash_table_remove_by_ins(&schedule->m_dns_querys, query);

    TAILQ_INSERT_TAIL(&schedule->m_free_dns_querys, query, m_next);
}

void net_dns_query_free_by_ctx(net_schedule_t schedule, void * ctx) {
    struct cpe_hash_it query_it;
    net_dns_query_t query;

    cpe_hash_it_init(&query_it, &schedule->m_dns_querys);

    query = cpe_hash_it_next(&query_it);
    while(query) {
        net_dns_query_t next = cpe_hash_it_next(&query_it);
        if (query->m_ctx == ctx) {
            net_dns_query_free(query);
        }
        query = next;
    }
}

void net_dns_query_free_all(net_schedule_t schedule) {
    struct cpe_hash_it query_it;
    net_dns_query_t query;

    cpe_hash_it_init(&query_it, &schedule->m_dns_querys);

    query = cpe_hash_it_next(&query_it);
    while(query) {
        net_dns_query_t next = cpe_hash_it_next(&query_it);
        net_dns_query_free(query);
        query = next;
    }
}

void net_dns_query_real_free(net_dns_query_t query) {
    net_schedule_t schedule = query->m_schedule;

    TAILQ_REMOVE(&schedule->m_free_dns_querys, query, m_next);
    mem_free(schedule->m_alloc, query);
}

void net_dns_query_notify_result_and_free(net_dns_query_t query, net_address_t main_address, net_address_it_t all_address) {
    assert(!query->m_processing);
    query->m_callback(query->m_ctx, main_address, all_address);
    net_dns_query_free(query);
}

void * net_dns_query_data(net_dns_query_t query) {
    return query + 1;
}

net_dns_query_t net_dns_query_from_data(void * data) {
    return ((net_dns_query_t)data) - 1;
}

const char * net_dns_query_type_str(net_dns_query_type_t type) {
    switch(type) {
    case net_dns_query_ipv4:
        return "ipv4";
    case net_dns_query_ipv6:
        return "ipv6";
    case net_dns_query_ipv4v6:
        return "ipv4v6";
    case net_dns_query_domain:
        return "domain";
    }
}

uint32_t net_dns_query_hash(net_dns_query_t o, void * user_data) {
    return o->m_query_id;
}

int net_dns_query_eq(net_dns_query_t l, net_dns_query_t r, void * user_data) {
    return l->m_query_id == r->m_query_id;
}
