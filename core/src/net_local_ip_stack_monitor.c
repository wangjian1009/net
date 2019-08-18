#include "net_local_ip_stack_monitor_i.h"
#include "net_driver_i.h"

net_local_ip_stack_monitor_t
net_local_ip_stack_monitor_create(
    net_schedule_t schedule,
    void * ctx,
    void (*ctx_free)(void * ctx),
    net_local_ip_stack_changed_fun_t on_change)
{
    net_local_ip_stack_monitor_t monitor = TAILQ_FIRST(&schedule->m_free_local_ip_stack_monitors);
    if (monitor) {
        TAILQ_REMOVE(&schedule->m_free_local_ip_stack_monitors, monitor, m_next);
    }
    else {
        monitor = mem_alloc(schedule->m_alloc, sizeof(struct net_local_ip_stack_monitor));
        if (monitor == NULL) {
            CPE_ERROR(schedule->m_em, "core: ip_stack monitor alloc fail!");
            return NULL;
        }
    }

    monitor->m_schedule = schedule;
    monitor->m_ctx = ctx;
    monitor->m_ctx_free = ctx_free;
    monitor->m_on_change = on_change;
    monitor->m_is_processing = 0;
    monitor->m_is_free = 0;

    TAILQ_INSERT_TAIL(&schedule->m_local_ip_stack_monitors, monitor, m_next);
    return monitor;
}

void net_local_ip_stack_monitor_free(net_local_ip_stack_monitor_t monitor) {
    net_schedule_t schedule = monitor->m_schedule;
    
    if (monitor->m_is_processing) {
        monitor->m_is_free = 1;
        return;
    }

    if (monitor->m_ctx_free) monitor->m_ctx_free(monitor->m_ctx);

    TAILQ_REMOVE(&schedule->m_local_ip_stack_monitors, monitor, m_next);

    TAILQ_INSERT_TAIL(&schedule->m_free_local_ip_stack_monitors, monitor, m_next);
}

void net_local_ip_stack_monitor_real_free(net_local_ip_stack_monitor_t monitor) {
    net_schedule_t schedule = monitor->m_schedule;
    TAILQ_REMOVE(&schedule->m_free_local_ip_stack_monitors, monitor, m_next);
    mem_free(schedule->m_alloc, monitor);
}

void net_local_ip_stack_monitor_free_by_ctx(net_schedule_t schedule, void * ctx) {
    net_local_ip_stack_monitor_t monitor, next_monitor;

    for(monitor = TAILQ_FIRST(&schedule->m_local_ip_stack_monitors); monitor; monitor = next_monitor) {
        next_monitor = TAILQ_NEXT(monitor, m_next);

        if (monitor->m_ctx == ctx) {
            net_local_ip_stack_monitor_free(monitor);
        }
    }
}
