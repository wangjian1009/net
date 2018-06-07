#ifndef NET_DNS_TASK_H_INCLEDED
#define NET_DNS_TASK_H_INCLEDED
#include "net_dns_system.h"

NET_BEGIN_DECL

net_dns_task_t net_dns_task_create(net_dns_manage_t manage, net_dns_entry_t entry);
void net_dns_task_free(net_dns_task_t task);

net_dns_task_t net_dns_task_find(net_dns_manage_t manage, net_dns_entry_t entry);

NET_END_DECL

#endif
