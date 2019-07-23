#ifndef NET_ICMP_PING_TASK_I_H_INCLEDED
#define NET_ICMP_PING_TASK_I_H_INCLEDED
#include "net_icmp_ping_task.h"
#include "net_icmp_mgr_i.h"

struct net_icmp_ping_task {
    net_icmp_mgr_t m_mgr;
    TAILQ_ENTRY(net_icmp_ping_task) m_next;
    net_icmp_ping_task_state_t m_state;
    net_icmp_ping_record_list_t m_records;
    net_icmp_ping_processor_t m_processor;
};

void net_icmp_ping_task_real_free(net_icmp_ping_task_t task);

#endif

