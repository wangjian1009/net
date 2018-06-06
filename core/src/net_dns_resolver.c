#include "cpe/pal/pal_strings.h"
#include "net_dgram.h"
#include "net_dns_resolver_i.h"
#include "net_dns_query_i.h"
#include "net_dns_server_i.h"

static void net_dns_dgram_process(
    net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source);

net_dns_resolver_t
net_dns_resolver_create(net_schedule_t schedule, net_dns_mode_t mode) {
    net_dns_resolver_t resolver = mem_alloc(schedule->m_alloc, sizeof(struct net_dns_resolver));
    if (resolver == NULL) {
        CPE_ERROR(schedule->m_em, "resolver: alloc fail");
        return NULL;
    }
    
    bzero(resolver, sizeof(*resolver));

    resolver->m_schedule = schedule;
    resolver->m_mode = mode;
    resolver->m_max_query_id = 0;
    TAILQ_INIT(&resolver->m_servers);

    resolver->m_dgram = net_dgram_create(schedule->m_direct_driver, NULL, net_dns_dgram_process, resolver);
    if (resolver->m_dgram == NULL) {
        CPE_ERROR(schedule->m_em, "resolver: create dgram fail!");
        mem_free(schedule->m_alloc, resolver);
        return NULL;
    }

    if (cpe_hash_table_init(
            &resolver->m_querys,
            schedule->m_alloc,
            (cpe_hash_fun_t) net_dns_query_hash,
            (cpe_hash_eq_t) net_dns_query_eq,
            CPE_HASH_OBJ2ENTRY(net_dns_query, m_hh),
            -1) != 0)
    {
        net_dgram_free(resolver->m_dgram);
        mem_free(schedule->m_alloc, resolver);
        return NULL;
    }
    
    schedule->m_dns_resolver = resolver;
    return resolver;
}

void net_dns_resolver_free(net_dns_resolver_t resolver) {
    net_schedule_t schedule = resolver->m_schedule;

    net_dns_query_free_all(resolver);
    cpe_hash_table_fini(&resolver->m_querys);

    while(!TAILQ_EMPTY(&resolver->m_servers)) {
        net_dns_server_free(TAILQ_FIRST(&resolver->m_servers));
    }

    net_dgram_free(resolver->m_dgram);
    resolver->m_dgram = NULL;

    if (schedule->m_dns_resolver == resolver) {
        schedule->m_dns_resolver = NULL;
    }

    mem_free(schedule->m_alloc, resolver);
}

net_dns_resolver_t net_dns_resolver_get(net_schedule_t schedule) {
    return schedule->m_dns_resolver;
}

net_dns_mode_t net_dns_resolver_mode(net_dns_resolver_t resolver) {
    return resolver->m_mode;
}

static void net_dns_dgram_process(
    net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t source)
{
    
}
