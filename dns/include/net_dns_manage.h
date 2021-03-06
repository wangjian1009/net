#ifndef NET_DNS_MANAGE_H_INCLEDED
#define NET_DNS_MANAGE_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_dns_system.h"

NET_BEGIN_DECL

net_dns_manage_t net_dns_manage_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule);

void net_dns_manage_free(net_dns_manage_t dns_manage);

uint8_t net_dns_manage_debug(net_dns_manage_t manage);
void net_dns_manage_set_debug(net_dns_manage_t manage, uint8_t debug);

uint32_t net_dns_manage_ttl_s(net_dns_manage_t manage);

void net_dns_manage_clear_cache(net_dns_manage_t manage);

net_dns_task_builder_t net_dns_task_builder_internal(net_dns_manage_t manage);

net_dns_task_builder_t net_dns_task_builder_default(net_dns_manage_t manage);
void net_dns_task_builder_set_default(net_dns_manage_t manage, net_dns_task_builder_t builder);

int net_dns_manage_auto_conf(
    net_dns_manage_t manage, net_driver_t driver, net_dns_scope_t scope,
    uint16_t timeout_s, uint16_t retry_count);

int net_dns_manage_add_record(
    net_dns_manage_t manage, net_address_t hostname, net_address_t address, uint32_t ttl_s);

void net_dns_hostnames_by_ip(net_address_it_t address_it, net_dns_manage_t manage, net_address_t ip);
net_address_t net_dns_hostname_by_ip(net_dns_manage_t manage, net_address_t ip);

NET_END_DECL

#endif
