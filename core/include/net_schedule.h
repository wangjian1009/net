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

net_address_matcher_t net_schedule_direct_matcher_white(net_schedule_t schedule);
net_address_matcher_t net_schedule_direct_matcher_white_check_create(net_schedule_t schedule);

net_address_matcher_t net_schedule_direct_matcher_black(net_schedule_t schedule);
net_address_matcher_t net_schedule_direct_matcher_black_check_create(net_schedule_t schedule);

void net_schedule_set_data_monitor(
    net_schedule_t schedule, net_data_monitor_fun_t monitor_fun, void * monitor_ctx);

/*dns*/
typedef int (*net_schedule_dns_query_start_fun_t)(void * ctx, net_dns_query_t query, const char * hostname);
typedef void (*net_schedule_dns_query_cancel_fun_t)(void * ctx, net_dns_query_t query);

void net_schedule_set_dns_resolver(
    net_schedule_t schedule,
    void * ctx,
    void (*ctx_fini)(void * ctx),
    uint16_t dns_query_capacity,
    net_schedule_dns_query_start_fun_t start_fun,
    net_schedule_dns_query_cancel_fun_t cancel_fun);

void * net_schedule_dns_resolver(net_schedule_t schedule);
    
NET_END_DECL

#endif
