#ifndef NET_LOCAL_IP_STACK_MONITOR_I_H_INCLEDED
#define NET_LOCAL_IP_STACK_MONITOR_I_H_INCLEDED
#include "net_local_ip_stack_monitor.h"
#include "net_schedule_i.h"

NET_BEGIN_DECL

struct net_local_ip_stack_monitor {
    net_schedule_t m_schedule;
    TAILQ_ENTRY(net_local_ip_stack_monitor) m_next;
    void * m_ctx;
    void (*m_ctx_free)(void *);
    net_local_ip_stack_changed_fun_t m_on_change;
    uint8_t m_is_processing;
    uint8_t m_is_free;
};

void net_local_ip_stack_monitor_real_free(net_local_ip_stack_monitor_t monitor);

NET_END_DECL

#endif
