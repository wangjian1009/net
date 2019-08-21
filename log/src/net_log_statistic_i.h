#ifndef NET_LOG_STATISTIC_I_H_INCLEDED
#define NET_LOG_STATISTIC_I_H_INCLEDED
#include "net_log_schedule_i.h"

void net_log_statistic_package_success(net_log_thread_t log_thread, net_log_category_t category);
void net_log_statistic_package_discard(net_log_thread_t log_thread, net_log_category_t category, net_log_discard_reason_t reason);
void net_log_statistic_cache_created(net_log_thread_t log_thread, net_log_category_t category);
void net_log_statistic_cache_destoried(net_log_thread_t log_thread, net_log_category_t category);

#endif
