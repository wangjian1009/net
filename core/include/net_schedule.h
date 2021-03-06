#ifndef NET_CONNECTOR_H_INCLEDED
#define NET_CONNECTOR_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

enum net_mem_type {
    net_mem_type_native,
    net_mem_type_cache,
    net_mem_type_ringbuffer,
};

struct net_mem_policy {
    net_mem_type_t m_type;
    union {
        uint64_t m_capacity;
    } m_ringbuffer;
};

net_schedule_t net_schedule_create(mem_allocrator_t alloc, error_monitor_t em, net_mem_policy_t mem_policy);
void net_schedule_free(net_schedule_t schedule);

mem_allocrator_t net_schedule_allocrator(net_schedule_t schedule);
error_monitor_t net_schedule_em(net_schedule_t schedule);
net_mem_group_t net_schedule_dft_mem_group(net_schedule_t schedule);
mem_buffer_t net_schedule_tmp_buffer(net_schedule_t schedule);

uint8_t net_schedule_debug(net_schedule_t schedule);
void net_schedule_set_debug(net_schedule_t schedule, uint8_t debug);

net_protocol_t net_schedule_noop_protocol(net_schedule_t schedule);
net_protocol_t net_schedule_null_protocol(net_schedule_t schedule);

uint32_t net_schedule_next_endpoint_id(net_schedule_t schedule);

uint8_t net_schedule_is_domain_address_valid(net_schedule_t schedule, const char * str_address);
uint8_t net_schedule_is_domain_address_arpa(net_schedule_t schedule, const char * str_address, net_address_t * address);

int64_t net_schedule_cur_time_ms(net_schedule_t schedule);

/*dns*/
typedef int (*net_schedule_dns_local_query_fun_t)(void * ctx, net_address_t hostname, net_address_it_t resolved_it, uint8_t recursive);
typedef int (*net_schedule_dns_query_init_fun_t)(
    void * ctx, net_dns_query_t query, net_address_t hostname, net_dns_query_type_t query_type, const char * policy);
typedef void (*net_schedule_dns_query_fini_fun_t)(void * ctx, net_dns_query_t query);

void net_schedule_set_dns_resolver(
    net_schedule_t schedule,
    void * ctx,
    void (*ctx_fini)(void * ctx),
    net_schedule_dns_local_query_fun_t local_query,
    uint16_t dns_query_capacity,
    net_schedule_dns_query_init_fun_t query_init,
    net_schedule_dns_query_fini_fun_t query_fini);

void * net_schedule_dns_resolver(net_schedule_t schedule);

int net_schedule_dns_local_query(
    net_schedule_t schedule,
    net_address_t hostname, net_address_it_t resolved_it, uint8_t recursive);

/*ipstack*/
net_local_ip_stack_t net_schedule_local_ip_stack(net_schedule_t schedule);
void net_schedule_set_local_ip_stack(net_schedule_t schedule, net_local_ip_stack_t ipstack);
int net_schedule_local_ip_stack_detect(net_schedule_t schedule);
const char * net_local_ip_stack_str(net_local_ip_stack_t ipstack);

/*progress*/
net_progress_path_search_policy_t net_schedule_progress_path_search_policy(net_schedule_t schedule);
void net_schedule_progress_set_path_search_policy(net_schedule_t schedule, net_progress_path_search_policy_t policy);

const char * net_schedule_progress_search_path(net_schedule_t schedule);
int net_schedule_progress_set_search_path(net_schedule_t schedule, const char * path);

/*debug*/
int net_debug_remote_host(net_schedule_t schedule, const char * remote, uint8_t protocol_debug, uint8_t driver_debug);
int net_debug_local_host(net_schedule_t schedule, const char * local, uint8_t protocol_debug, uint8_t driver_debug);
int net_debug_protocol(net_schedule_t schedule, const char * protocol, uint8_t debug);
int net_debug_driver(net_schedule_t schedule, const char * protocol, uint8_t debug);

/*utils*/
const char * net_mem_type_str(net_mem_type_t mem_type);

NET_END_DECL

#endif
