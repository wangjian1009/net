#ifndef NET_DNS_TASK_MONITOR_I_H_INCLEDED
#define NET_DNS_TASK_MONITOR_I_H_INCLEDED
#include "net_dns_task_monitor.h"
#include "net_dns_manage_i.h"

NET_BEGIN_DECL

struct net_dns_task_monitor {
    net_dns_manage_t m_manage;
    TAILQ_ENTRY(net_dns_task_monitor) m_next;
    void * m_ctx;
    net_dns_task_on_complete_fun_t m_on_complete;
};

NET_END_DECL

#endif
