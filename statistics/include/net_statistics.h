#ifndef NET_STATISTICS_H_INCLEDED
#define NET_STATISTICS_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_statistics_system.h"

CPE_BEGIN_DECL

net_statistics_t net_statistics_create(mem_allocrator_t alloc, error_monitor_t em);
void net_statistics_free(net_statistics_t statistics);

uint8_t net_statistics_debug(net_statistics_t statistics);
void net_statistics_set_debug(net_statistics_t statistics, uint8_t debug);

void net_statistics_log_metric_for_count(net_statistics_t statistics, const char *name, int quantity);
void net_statistics_log_metric_for_duration(net_statistics_t statistics, const char *name, uint64_t duration_ms);

void net_statistics_log_event(net_statistics_t statistics, const char *type, const char *name, const char *status, const char *data);

CPE_END_DECL

#endif
