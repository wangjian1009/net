#ifndef NET_DNS_TASK_H_INCLEDED
#define NET_DNS_TASK_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_dns_system.h"

NET_BEGIN_DECL

net_address_t net_dns_task_hostname(net_dns_task_t task);
const char * net_dns_task_hostname_str(net_dns_task_t task);
net_dns_query_type_t net_dns_task_query_type(net_dns_task_t task);

net_dns_task_state_t net_dns_task_state(net_dns_task_t task);
int net_dns_task_start(net_dns_task_t task);

net_dns_task_step_t net_dns_task_step_current(net_dns_task_t task);
void net_dns_task_steps(net_dns_task_t task, net_dns_task_step_it_t it);
uint8_t net_dns_task_is_complete(net_dns_task_t task);

int64_t net_dns_task_begin_time_ms(net_dns_task_t task);
int64_t net_dns_task_complete_time_ms(net_dns_task_t task);

const char * net_dns_task_state_str(net_dns_task_state_t state);
uint8_t net_dns_task_state_is_complete(net_dns_task_state_t state);

void net_dns_task_print(write_stream_t ws, net_dns_task_t task);
const char * net_dns_task_dump(mem_buffer_t buffer, net_dns_task_t task);

uint8_t net_dns_query_type_is_support_by_ip_stack(
    net_dns_query_type_t query_type, net_local_ip_stack_t ip_stack);

NET_END_DECL

#endif
