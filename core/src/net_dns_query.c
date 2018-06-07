#include "net_dns_query_i.h"

net_dns_query_t net_dns_query_create(
    net_dns_resolver_t resolver,
    const char *hostname,
    net_dns_query_callback_fun_t callback,
    net_dns_query_ctx_free_fun_t ctx_free,
    void * ctx)
{
    net_schedule_t schedule = resolver->m_schedule;

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

    query->m_resolver = resolver;
    query->m_query_id = resolver->m_max_query_id++;
    query->m_callback = callback;
    query->m_ctx_free = ctx_free;
    query->m_ctx = ctx;
    
    return query;
}

void net_dns_query_free(net_dns_query_t query) {
    net_dns_resolver_t resolver = query->m_resolver;
    net_schedule_t schedule = resolver->m_schedule;

    if (query->m_ctx_free) {
        query->m_ctx_free(query->m_ctx);
    }
    
    cpe_hash_table_remove_by_ins(&resolver->m_querys, query);

    query->m_resolver = (net_dns_resolver_t)schedule;
    TAILQ_INSERT_TAIL(&schedule->m_free_dns_querys, query, m_next);
}

void net_dns_query_free_all(net_dns_resolver_t resolver) {
    struct cpe_hash_it query_it;
    net_dns_query_t query;

    cpe_hash_it_init(&query_it, &resolver->m_querys);

    query = cpe_hash_it_next(&query_it);
    while(query) {
        net_dns_query_t next = cpe_hash_it_next(&query_it);
        net_dns_query_free(query);
        query = next;
    }
}

void net_dns_query_real_free(net_dns_query_t query) {
    net_schedule_t schedule = (net_schedule_t)query->m_resolver;

    TAILQ_REMOVE(&schedule->m_free_dns_querys, query, m_next);
    mem_free(schedule->m_alloc, query);
}

uint32_t net_dns_query_hash(net_dns_query_t o) {
    return o->m_query_id;
}

int net_dns_query_eq(net_dns_query_t l, net_dns_query_t r) {
    return l->m_query_id == r->m_query_id;
}
