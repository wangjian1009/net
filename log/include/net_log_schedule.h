#ifndef NET_LOG_SCHEDULE_H_INCLEDED
#define NET_LOG_SCHEDULE_H_INCLEDED
#include "net_log_types.h"

NET_BEGIN_DECL

net_log_schedule_t net_log_schedule_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule, net_driver_t driver);

void net_log_schedule_free(net_log_schedule_t log_schedule);

NET_END_DECL

#endif
