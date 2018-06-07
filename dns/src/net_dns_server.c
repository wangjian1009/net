#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "net_dgram.h"
#include "net_address.h"
#include "net_dns_server_i.h"

net_dns_server_t
net_dns_server_create(net_dns_manage_t manage, net_address_t addr) {
    net_schedule_t schedule = manage->m_schedule;

    net_dns_server_t server = mem_alloc(manage->m_alloc, sizeof(struct net_dns_server));
    if (server == NULL) {
        CPE_ERROR(manage->m_em, "dns_server_create: alloc fail!");
        return NULL;
    }

    server->m_manage = manage;
    server->m_address = addr;

    net_address_free(addr);
    TAILQ_INSERT_TAIL(&manage->m_servers, server, m_next);
    return server;
}

void net_dns_server_free(net_dns_server_t server) {
    net_dns_manage_t manage = server->m_manage;

    TAILQ_REMOVE(&manage->m_servers, server, m_next);
    mem_free(manage->m_alloc, server);
}

