#include "net_log_statistic_monitor_i.h"

net_log_statistic_monitor_t
net_log_statistic_monitor_create(
    net_log_schedule_t schedule, 
    net_log_statistic_on_discard_fun_t on_discard, 
    net_log_statistic_on_op_error_fun_t on_op_error, 
    void * monitor_ctx)
{
    net_log_statistic_monitor_t monitor = mem_alloc(schedule->m_alloc, sizeof(struct net_log_statistic_monitor));
    if (monitor == NULL) {
        CPE_ERROR(schedule->m_em, "log: statistic monitor: alloc fail!");
        return NULL;
    }

    monitor->m_schedule = schedule;
    monitor->m_on_discard = on_discard;
    monitor->m_on_op_error = on_op_error;
    monitor->m_monitor_ctx = monitor_ctx;

    TAILQ_INSERT_TAIL(&schedule->m_statistic_monitors, monitor, m_next);
    
    return monitor;
}

void net_log_statistic_monitor_free(net_log_statistic_monitor_t monitor) {
    net_log_schedule_t schedule = monitor->m_schedule;

    TAILQ_REMOVE(&schedule->m_statistic_monitors, monitor, m_next);

    mem_free(schedule->m_alloc, monitor);
}

