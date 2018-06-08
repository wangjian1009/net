#ifndef NET_DNS_MANAGE_H_INCLEDED
#define NET_DNS_MANAGE_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_dns_system.h"

NET_BEGIN_DECL

net_dns_manage_t net_dns_manage_create(
    mem_allocrator_t alloc, error_monitor_t em,
    net_schedule_t schedule, net_driver_t driver);

void net_dns_manage_free(net_dns_manage_t dns_manage);

net_dns_mode_t net_dns_manager_mode(net_dns_manage_t manage);
void net_dns_manager_set_mode(net_dns_manage_t manage, net_dns_mode_t mode);

uint8_t net_dns_manage_debug(net_dns_manage_t manage);
void net_dns_manage_set_debug(net_dns_manage_t manage, uint8_t debug);

net_dns_task_builder_t net_dns_task_builder_internal(net_dns_manage_t manage);

net_dns_task_builder_t net_dns_task_builder_default(net_dns_manage_t manage);
void net_dns_task_builder_set_default(net_dns_manage_t manage, net_dns_task_builder_t builder);

NET_END_DECL

#endif
