#include "net_log_state_monitor_i.h"

net_log_state_monitor_t
net_log_state_monitor_create(net_log_schedule_t schedule, net_log_state_monitor_fun_t monitor_fun, void * monitor_ctx) {
    net_log_state_monitor_t monitor = mem_alloc(schedule->m_alloc, sizeof(struct net_log_state_monitor));
    if (monitor == NULL) {
        CPE_ERROR(schedule->m_em, "log: state monitor: alloc fail!");
        return NULL;
    }

    monitor->m_schedule = schedule;
    monitor->m_monitor_fun = monitor_fun;
    monitor->m_monitor_ctx = monitor_ctx;

    TAILQ_INSERT_TAIL(&schedule->m_state_monitors, monitor, m_next);
    
    return monitor;
}

void net_log_state_monitor_free(net_log_state_monitor_t monitor) {
    net_log_schedule_t schedule = monitor->m_schedule;

    TAILQ_REMOVE(&schedule->m_state_monitors, monitor, m_next);

    mem_free(schedule->m_alloc, monitor);
}

