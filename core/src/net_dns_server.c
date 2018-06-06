#include "net_dgram.h"
#include "net_address.h"
#include "net_dns_server_i.h"

net_dns_server_t
net_dns_server_create(net_dns_resolver_t resolver, net_address_t addr) {
    net_schedule_t schedule = resolver->m_schedule;

    net_dns_server_t server = mem_alloc(schedule->m_alloc, sizeof(struct net_dns_server));
    if (server == NULL) {
        CPE_ERROR(schedule->m_em, "dns_server_create: alloc fail!");
        return NULL;
    }

    server->m_resolver = resolver;
    server->m_address = addr;

    net_address_free(addr);
    TAILQ_INSERT_TAIL(&resolver->m_servers, server, m_next);
    return server;
}

void net_dns_server_free(net_dns_server_t server) {
    net_dns_resolver_t resolver = server->m_resolver;

    TAILQ_REMOVE(&resolver->m_servers, server, m_next);
    mem_free(resolver->m_schedule->m_alloc, server);
}

