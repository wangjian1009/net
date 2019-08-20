#ifndef NET_LOG_STATISTIC_MONITOR_H_INCLEDED
#define NET_LOG_STATISTIC_MONITOR_H_INCLEDED
#include "net_trans_system.h" 
#include "net_log_types.h"

NET_BEGIN_DECL

typedef void (*net_log_statistic_on_discard_fun_t)(
    void * ctx, net_log_category_t category, net_log_discard_reason_t reason);

typedef void (*net_log_statistic_on_op_error_fun_t)(
    void * ctx, net_log_env_t env, net_log_category_t category, 
    net_trans_task_error_t trans_error, int http_code, const char * http_msg);

net_log_statistic_monitor_t
net_log_statistic_monitor_create(
    net_log_schedule_t schedule, 
    net_log_statistic_on_discard_fun_t on_discard,
    net_log_statistic_on_op_error_fun_t on_op_error,
    void * monitor_ctx);

void net_log_statistic_monitor_free(net_log_statistic_monitor_t monitor);

NET_END_DECL

#endif
