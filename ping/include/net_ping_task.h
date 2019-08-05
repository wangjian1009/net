#ifndef NET_PING_TASK_H_INCLEDED
#define NET_PING_TASK_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_ping_types.h"

NET_BEGIN_DECL

enum net_ping_type {
    net_ping_type_icmp,
    net_ping_type_tcp_connect,
    net_ping_type_http,
};

typedef void (*net_ping_task_cb_fun_t)(void * ctx, net_ping_task_t task, net_ping_record_t record);

net_ping_task_t net_ping_task_create_icmp(net_ping_mgr_t mgr, net_address_t target);
net_ping_task_t net_ping_task_create_tcp_connect(net_ping_mgr_t mgr, net_address_t target);
net_ping_task_t net_ping_task_create_http(net_ping_mgr_t mgr, const char * url);

void net_ping_task_free(net_ping_task_t task);

net_ping_type_t net_pint_task_type(net_ping_task_t task);
net_address_t net_ping_task_target(net_ping_task_t task);
net_ping_task_state_t net_ping_task_state(net_ping_task_t task);
net_ping_error_t net_ping_task_error(net_ping_task_t task);
void net_ping_task_records(net_ping_task_t task, net_ping_record_it_t record_it);
void net_ping_task_set_cb(net_ping_task_t task, void * cb_ctx, net_ping_task_cb_fun_t cb_fun);

void net_ping_task_start(net_ping_task_t task, uint32_t ping_span_ms, uint16_t ping_count);

uint32_t net_ping_task_ping_max(net_ping_task_t task);
uint32_t net_ping_task_ping_min(net_ping_task_t task);
uint32_t net_ping_task_ping_avg(net_ping_task_t task);

void net_ping_task_print(write_stream_t ws, net_ping_task_t task);
const char * net_ping_task_dump(mem_buffer_t buffer, net_ping_task_t task);

const char * net_ping_task_state_str(net_ping_task_state_t state);
const char * net_ping_error_str(net_ping_error_t error);

NET_END_DECL

#endif
