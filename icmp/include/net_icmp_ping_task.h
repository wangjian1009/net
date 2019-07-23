#ifndef NET_ICMP_PING_TASK_H_INCLEDED
#define NET_ICMP_PING_TASK_H_INCLEDED
#include "net_icmp_types.h"

NET_BEGIN_DECL

enum net_icmp_ping_task_state {
    net_icmp_ping_task_state_init,
    net_icmp_ping_task_state_processing,
    net_icmp_ping_task_state_done,
};

net_icmp_ping_task_t net_icmp_ping_task_create(net_icmp_mgr_t mgr);
void net_icmp_ping_task_free(net_icmp_ping_task_t task);

net_icmp_ping_task_state_t net_icmp_ping_task_state(net_icmp_ping_task_t task);
void net_icmp_ping_task_records(net_icmp_ping_task_t task, net_icmp_ping_record_it_t record_it);

int net_icmp_ping_task_start(net_icmp_ping_task_t task, net_address_t target, uint16_t ping_count);

uint32_t net_icmp_ping_task_ping_max(net_icmp_ping_task_t task);
uint32_t net_icmp_ping_task_ping_min(net_icmp_ping_task_t task);
uint32_t net_icmp_ping_task_ping_avg(net_icmp_ping_task_t task);

const char * net_icmp_ping_task_state_str(net_icmp_ping_task_state_t state);

NET_END_DECL

#endif
