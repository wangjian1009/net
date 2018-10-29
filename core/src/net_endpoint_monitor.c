#include "net_endpoint_monitor_i.h"
#include "net_driver_i.h"

net_endpoint_monitor_t
net_endpoint_monitor_create(
    net_endpoint_t endpoint,
    void * ctx,
    void (*ctx_free)(void * ctx),
    net_endpoint_monitor_state_changed_fun_t on_state_change)
{
    net_schedule_t schedule = endpoint->m_driver->m_schedule;
    
    net_endpoint_monitor_t monitor = TAILQ_FIRST(&schedule->m_free_endpoint_monitors);
    if (monitor) {
        TAILQ_REMOVE(&schedule->m_free_endpoint_monitors, monitor, m_next);
    }
    else {
        monitor = mem_alloc(schedule->m_alloc, sizeof(struct net_endpoint_monitor));
        if (monitor == NULL) {
            CPE_ERROR(schedule->m_em, "core: endpoint monitor alloc fail!");
            return NULL;
        }
    }

    monitor->m_endpoint = endpoint;
    monitor->m_ctx = ctx;
    monitor->m_ctx_free = ctx_free;
    monitor->m_on_state_change = on_state_change;
    monitor->m_is_processing = 0;
    monitor->m_is_free = 0;

    TAILQ_INSERT_TAIL(&endpoint->m_monitors, monitor, m_next);
    return monitor;
}

void net_endpoint_monitor_free(net_endpoint_monitor_t monitor) {
    net_endpoint_t endpoint = monitor->m_endpoint;
    net_schedule_t schedule = endpoint->m_driver->m_schedule;
    
    if (monitor->m_is_processing) {
        monitor->m_is_free = 1;
        return;
    }

    if (monitor->m_ctx_free) monitor->m_ctx_free(monitor->m_ctx);

    TAILQ_REMOVE(&endpoint->m_monitors, monitor, m_next);

    monitor->m_endpoint = (net_endpoint_t)schedule;
    TAILQ_INSERT_TAIL(&schedule->m_free_endpoint_monitors, monitor, m_next);
}

void net_endpoint_monitor_real_free(net_endpoint_monitor_t monitor) {
    net_schedule_t schedule = (net_schedule_t)monitor->m_endpoint;
    TAILQ_REMOVE(&schedule->m_free_endpoint_monitors, monitor, m_next);
    mem_free(schedule->m_alloc, monitor);
}

void net_endpoint_monitor_free_by_ctx(net_endpoint_t endpoint, void * ctx) {
    net_endpoint_monitor_t monitor, next_monitor;

    for(monitor = TAILQ_FIRST(&endpoint->m_monitors); monitor; monitor = next_monitor) {
        next_monitor = TAILQ_NEXT(monitor, m_next);

        if (monitor->m_ctx == ctx) {
            net_endpoint_monitor_free(monitor);
        }
    }
}
