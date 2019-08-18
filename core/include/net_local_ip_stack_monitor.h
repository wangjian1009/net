#ifndef NET_LOCAL_IP_STACK_MONITOR_H_INCLEDED
#define NET_LOCAL_IP_STACK_MONITOR_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef void (*net_local_ip_stack_changed_fun_t)(void * ctx, net_schedule_t net_schedule);

net_local_ip_stack_monitor_t
net_local_ip_stack_monitor_create(
    net_schedule_t schedule,
    void * ctx,
    void (*ctx_free)(void * ctx),
    net_local_ip_stack_changed_fun_t changed_fun);

void net_local_ip_stack_monitor_free(net_local_ip_stack_monitor_t ip_stack);

void net_local_ip_stack_monitor_free_by_ctx(net_schedule_t schedule, void * ctx);

NET_END_DECL

#endif
