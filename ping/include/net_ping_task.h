#ifndef NET_PING_TASK_H_INCLEDED
#define NET_PING_TASK_H_INCLEDED
#include "net_ping_types.h"

NET_BEGIN_DECL

enum net_ping_task_state {
    net_ping_task_state_init,
    net_ping_task_state_processing,
    net_ping_task_state_done,
    net_ping_task_state_error,
};

net_ping_task_t net_ping_task_create(net_ping_mgr_t mgr, net_address_t target);
void net_ping_task_free(net_ping_task_t task);

net_address_t net_ping_task_target(net_ping_task_t task);
net_ping_task_state_t net_ping_task_state(net_ping_task_t task);
void net_ping_task_records(net_ping_task_t task, net_ping_record_it_t record_it);

int net_ping_task_start(net_ping_task_t task, uint16_t ping_count);

uint32_t net_ping_task_ping_max(net_ping_task_t task);
uint32_t net_ping_task_ping_min(net_ping_task_t task);
uint32_t net_ping_task_ping_avg(net_ping_task_t task);

const char * net_ping_task_state_str(net_ping_task_state_t state);

NET_END_DECL

#endif
