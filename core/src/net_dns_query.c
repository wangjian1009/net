#include "net_dns_query_i.h"

net_dns_query_t net_dns_query_create(
    net_schedule_t schedule,
    const char *hostname,
    net_dns_query_callback_fun_t callback,
    net_dns_query_ctx_free_fun_t ctx_free,
    void * ctx)
{
    if (schedule->m_dns_resolver_ctx == NULL) {
        CPE_ERROR(schedule->m_em, "query_create: no dns resolver!");
        return NULL;
    }
    
    net_dns_query_t query = TAILQ_FIRST(&schedule->m_free_dns_querys);
    if (query) {
        TAILQ_REMOVE(&schedule->m_free_dns_querys, query, m_next);
    }
    else {
        query = mem_alloc(schedule->m_alloc, sizeof(struct net_dns_query));
        if (query == NULL) {
            CPE_ERROR(schedule->m_em, "query_create: alloc fail!");
            return NULL;
        }
    }

    query->m_schedule = schedule;
    query->m_query_id = schedule->m_dns_max_query_id++;
    query->m_callback = callback;
    query->m_ctx_free = ctx_free;
    query->m_ctx = ctx;
    
    return query;
}

void net_dns_query_free(net_dns_query_t query) {
    net_schedule_t schedule = query->m_schedule;

    if (query->m_ctx_free) {
        query->m_ctx_free(query->m_ctx);
    }
    
    cpe_hash_table_remove_by_ins(&schedule->m_dns_querys, query);

    TAILQ_INSERT_TAIL(&schedule->m_free_dns_querys, query, m_next);
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

uint32_t net_dns_query_hash(net_dns_query_t o) {
    return o->m_query_id;
}

int net_dns_query_eq(net_dns_query_t l, net_dns_query_t r) {
    return l->m_query_id == r->m_query_id;
}
