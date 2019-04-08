#ifndef NET_LOG_STATE_MONITOR_H_INCLEDED
#define NET_LOG_STATE_MONITOR_H_INCLEDED
#include "net_log_types.h"

NET_BEGIN_DECL

typedef void (*net_log_state_monitor_fun_t)(void * ctx, net_log_schedule_t schedule, net_log_schedule_state_t from_state);

net_log_state_monitor_t net_log_state_monitor_create(net_log_schedule_t schedule, net_log_state_monitor_fun_t monitor_fun, void * monitor_ctx);
void net_log_state_monitor_free(net_log_state_monitor_t monitor);

NET_END_DECL

#endif
