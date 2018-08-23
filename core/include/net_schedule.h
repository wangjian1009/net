#ifndef NET_CONNECTOR_H_INCLEDED
#define NET_CONNECTOR_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

net_schedule_t net_schedule_create(
    mem_allocrator_t alloc, error_monitor_t em, uint32_t common_buff_capacity);
void net_schedule_free(net_schedule_t schedule);

mem_allocrator_t net_schedule_allocrator(net_schedule_t schedule);
error_monitor_t net_schedule_em(net_schedule_t schedule);
mem_buffer_t net_schedule_tmp_buffer(net_schedule_t schedule);

uint8_t net_schedule_debug(net_schedule_t schedule);
void net_schedule_set_debug(net_schedule_t schedule, uint8_t debug);

net_protocol_t net_schedule_direct_protocol(net_schedule_t schedule);

net_driver_t net_schedule_direct_driver(net_schedule_t schedule);
void net_schedule_set_direct_driver(net_schedule_t schedule, net_driver_t driver);

/*dns*/
typedef int (*net_schedule_dns_query_init_fun_t)(void * ctx, net_dns_query_t query, const char * hostname, const char * policy);
typedef void (*net_schedule_dns_query_fini_fun_t)(void * ctx, net_dns_query_t query);

void net_schedule_set_dns_resolver(
    net_schedule_t schedule,
    void * ctx,
    void (*ctx_fini)(void * ctx),
    uint16_t dns_query_capacity,
    net_schedule_dns_query_init_fun_t query_init,
    net_schedule_dns_query_fini_fun_t query_fini);

void * net_schedule_dns_resolver(net_schedule_t schedule);

/*debug*/
int net_debug_remote_host(net_schedule_t schedule, const char * remote, uint8_t protocol_debug, uint8_t driver_debug);
int net_debug_local_host(net_schedule_t schedule, const char * local, uint8_t protocol_debug, uint8_t driver_debug);
int net_debug_protocol(net_schedule_t schedule, const char * protocol, uint8_t debug);
int net_debug_driver(net_schedule_t schedule, const char * protocol, uint8_t debug);

NET_END_DECL

#endif
