#include "net_address.h"
#include "net_driver_i.h"
#include "net_endpoint_next_i.h"

net_endpoint_next_t
net_endpoint_next_create(net_endpoint_t endpoint, net_address_t address) {
    net_schedule_t schedule = endpoint->m_driver->m_schedule;
    
    net_endpoint_next_t next = TAILQ_FIRST(&schedule->m_free_endpoint_nexts);
    if (next) {
        TAILQ_REMOVE(&schedule->m_free_endpoint_nexts, next, m_next);
    }
    else {
        next = mem_alloc(schedule->m_alloc, sizeof(struct net_endpoint_next));
        if (next == NULL) {
            CPE_ERROR(schedule->m_em, "core: endpoint next alloc fail!");
            return NULL;
        }
    }

    next->m_endpoint = endpoint;
    next->m_address = net_address_copy(schedule, address);
    if (next->m_address == NULL) {
        CPE_ERROR(schedule->m_em, "core: endpoint next alloc: dup address fail!");
        next->m_endpoint = (net_endpoint_t)schedule;
        TAILQ_INSERT_TAIL(&schedule->m_free_endpoint_nexts, next, m_next);
        return NULL;
    }

    TAILQ_INSERT_TAIL(&endpoint->m_nexts, next, m_next);
    return next;
}

void net_endpoint_next_free(net_endpoint_next_t next) {
    net_endpoint_t endpoint = next->m_endpoint;
    net_schedule_t schedule = endpoint->m_driver->m_schedule;

    if (next->m_address) {
        net_address_free(next->m_address);
        next->m_address = NULL;
    }

    TAILQ_REMOVE(&endpoint->m_nexts, next, m_next);

    next->m_endpoint = (net_endpoint_t)schedule;
    TAILQ_INSERT_TAIL(&schedule->m_free_endpoint_nexts, next, m_next);
}

void net_endpoint_next_real_free(net_endpoint_next_t next) {
    net_schedule_t schedule = (net_schedule_t)next->m_endpoint;
    TAILQ_REMOVE(&schedule->m_free_endpoint_nexts, next, m_next);
    mem_free(schedule->m_alloc, next);
}
