#ifndef NET_DNS_TASK_STEP_H_INCLEDED
#define NET_DNS_TASK_STEP_H_INCLEDED
#include "net_dns_system.h"

NET_BEGIN_DECL

struct net_dns_task_step_it {
    net_dns_task_step_t (*next)(struct net_dns_task_step_it * it);
    char m_data[64];
};

net_dns_task_step_t net_dns_task_step_create(net_dns_task_t task, net_dns_task_step_complete_policy_t complete_policy);
void net_dns_task_step_free(net_dns_task_step_t step);

net_dns_task_state_t net_dns_task_step_state(net_dns_task_step_t step);

void net_dns_task_step_ctxes(net_dns_task_step_t step, net_dns_task_ctx_it_t it);

#define net_dns_task_step_it_next(it) ((it)->next ? (it)->next(it) : NULL)

NET_END_DECL

#endif
