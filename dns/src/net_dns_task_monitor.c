#include "net_dns_task_monitor_i.h"

net_dns_task_monitor_t
net_dns_task_monitor_create(
    net_dns_manage_t manage,
    void * ctx,
    net_dns_task_on_complete_fun_t on_complete)
{
    net_dns_task_monitor_t monitor = mem_alloc(manage->m_alloc, sizeof(struct net_dns_task_monitor));
    if (monitor == NULL) {
        CPE_ERROR(manage->m_em, "dns-cli: monitor alloc fail");
        return NULL;
    }

    monitor->m_manage = manage;
    monitor->m_ctx = ctx;
    monitor->m_on_complete = on_complete;

    TAILQ_INSERT_TAIL(&manage->m_monitors, monitor, m_next);
    
    return monitor;
}

void net_dns_task_monitor_free(net_dns_task_monitor_t task_monitor) {
    net_dns_manage_t manage = task_monitor->m_manage;

    TAILQ_REMOVE(&manage->m_monitors, task_monitor, m_next);

    mem_free(manage->m_alloc, task_monitor);
}

