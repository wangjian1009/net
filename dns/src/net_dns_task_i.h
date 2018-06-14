#ifndef NET_DNS_TASK_I_H_INCLEDED
#define NET_DNS_TASK_I_H_INCLEDED
#include "net_dns_task.h"
#include "net_dns_manage_i.h"

NET_BEGIN_DECL

struct net_dns_task {
    net_dns_manage_t m_manage;
    net_dns_entry_t m_entry;
    TAILQ_ENTRY(net_dns_task) m_next;
    net_dns_task_state_t m_state;
    net_dns_task_step_t m_step_current;
    net_dns_task_step_list_t m_steps;
    net_dns_query_ex_list_t m_querys;    
};

void net_dns_task_real_free(net_dns_task_t task);

void net_dns_task_update_state(net_dns_task_t task);
net_dns_task_state_t net_dns_task_calc_state(net_dns_task_t task);

NET_END_DECL

#endif
