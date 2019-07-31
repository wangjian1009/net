#ifndef NET_EBB_SERVICE_H
#define NET_EBB_SERVICE_H
#include "cpe/utils/utils_types.h"
#include "net_ebb_system.h"

net_ebb_service_t net_ebb_service_create(net_schedule_t schedule, const char * protocol_name);
void net_ebb_service_free(net_ebb_service_t service);

net_protocol_t net_ebb_service_to_protocol(net_ebb_service_t service);

net_ebb_mount_point_t net_ebb_service_root(net_ebb_service_t service);

#endif
