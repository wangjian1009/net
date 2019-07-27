#ifndef NET_EBB_SERVICE_H
#define NET_EBB_SERVICE_H
#include "cpe/utils/utils_types.h"
#include "net_ebb_system.h"

#define EBB_AGAIN 0
#define EBB_STOP 1

net_ebb_service_t ebb_service_create(mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule, const char * name);
void net_ebb_service_free(net_ebb_service_t service);

net_protocol_t net_ebb_service_to_protocol(net_ebb_service_t service);

#endif
