#ifndef NET_DNS_TASK_I_H_INCLEDED
#define NET_DNS_TASK_I_H_INCLEDED
#include "net_dns_task.h"
#include "net_dns_manage_i.h"

NET_BEGIN_DECL

struct net_dns_task {
    net_dns_manage_t m_manage;
    cpe_hash_entry m_hh;
    net_dns_query_type_t m_query_type;
    net_address_t m_address;
    TAILQ_ENTRY(net_dns_task) m_next_for_manage;
    int64_t m_begin_time_ms;
    int64_t m_complete_time_ms;
    net_dns_task_state_t m_state;
    net_dns_task_step_t m_step_current;
    net_dns_task_step_list_t m_steps;
    net_dns_query_ex_list_t m_querys;    
};

net_dns_task_t net_dns_task_create(net_dns_manage_t manage, net_address_t address, net_dns_query_type_t query_type);
void net_dns_task_free(net_dns_task_t task);

void net_dns_task_real_free(net_dns_task_t task);

void net_dns_task_update_state(net_dns_task_t task, net_dns_task_state_t new_state);
net_dns_task_state_t net_dns_task_calc_state(net_dns_task_t task);

net_dns_task_t net_dns_task_find(net_dns_manage_t manage, net_address_t address, net_dns_query_type_t query_type); 

uint32_t net_dns_task_hash(net_dns_task_t o, void * user_data);
int net_dns_task_eq(net_dns_task_t l, net_dns_task_t r, void * user_data);

NET_END_DECL

#endif
