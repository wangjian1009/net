#ifndef NET_DNS_TASK_MONITOR_H_INCLEDED
#define NET_DNS_TASK_MONITOR_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_dns_system.h"

NET_BEGIN_DECL

typedef void (*net_dns_task_on_complete_fun_t)(void * ctx, net_dns_task_t task);

net_dns_task_monitor_t
net_dns_task_monitor_create(
    net_dns_manage_t manage,
    void * ctx,
    net_dns_task_on_complete_fun_t on_complete);

void net_dns_task_monitor_free(net_dns_task_monitor_t task_monitor);


NET_END_DECL

#endif
