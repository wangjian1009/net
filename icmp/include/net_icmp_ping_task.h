#ifndef NET_ICMP_PING_TASK_H_INCLEDED
#define NET_ICMP_PING_TASK_H_INCLEDED
#include "net_icmp_types.h"

NET_BEGIN_DECL

net_icmp_ping_tak_t net_icmp_ping_tak_create(net_schedule_t schedule);
void net_icmp_ping_tak_free(net_icmp_ping_tak_t task);

NET_END_DECL

#endif
