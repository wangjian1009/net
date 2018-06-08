#ifndef NET_DNS_TASK_STEP_H_INCLEDED
#define NET_DNS_TASK_STEP_H_INCLEDED
#include "net_dns_system.h"

NET_BEGIN_DECL

net_dns_task_step_t net_dns_task_step_create(net_dns_task_t task);
void net_dns_task_step_free(net_dns_task_step_t step);

net_dns_task_state_t net_dns_task_step_state(net_dns_task_step_t step);

NET_END_DECL

#endif
