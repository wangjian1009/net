#ifndef NET_ICMP_MGR_H_INCLEDED
#define NET_ICMP_MGR_H_INCLEDED
#include "net_icmp_types.h"

NET_BEGIN_DECL

net_icmp_mgr_t net_icmp_mgr_create(mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule);
void net_icmp_mgr_free(net_icmp_mgr_t mgr);

NET_END_DECL

#endif
