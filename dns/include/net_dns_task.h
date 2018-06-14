#ifndef NET_DNS_TASK_H_INCLEDED
#define NET_DNS_TASK_H_INCLEDED
#include "net_dns_system.h"

NET_BEGIN_DECL

net_dns_task_t net_dns_task_create(net_dns_manage_t manage, net_dns_entry_t entry);
void net_dns_task_free(net_dns_task_t task);

const char * net_dns_task_hostname(net_dns_task_t task);

net_dns_task_state_t net_dns_task_state(net_dns_task_t task);
int net_dns_task_start(net_dns_task_t task);

net_dns_task_step_t net_dns_task_step_current(net_dns_task_t task);
uint8_t net_dns_task_is_complete(net_dns_task_t task);

const char * net_dns_task_state_str(net_dns_task_state_t state);
uint8_t net_dns_task_state_is_complete(net_dns_task_state_t state);

NET_END_DECL

#endif
