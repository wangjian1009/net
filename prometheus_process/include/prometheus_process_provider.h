#ifndef PROMETHEUS_PROCESS_COLLECTOR_H_INCLEDED
#define PROMETHEUS_PROCESS_COLLECTOR_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "cpe/vfs/vfs_types.h"
#include "prometheus_process_types.h"

prometheus_process_provider_t
prometheus_process_provider_create(
    prometheus_manager_t manager,
    error_monitor_t em, mem_allocrator_t alloc, vfs_mgr_t vfs,
    const char * limits_path, const char *stat_path);

void prometheus_process_provider_free(
    prometheus_process_provider_t provider);

prometheus_metric_t prometheus_process_provider_cpu_seconds_total(prometheus_process_provider_t provider);
prometheus_metric_t prometheus_process_provider_virtual_memory_bytes(prometheus_process_provider_t provider);
prometheus_metric_t prometheus_process_provider_resident_memory_bytes(prometheus_process_provider_t provider);
prometheus_metric_t prometheus_process_provider_start_time_seconds(prometheus_process_provider_t provider);
prometheus_metric_t prometheus_process_provider_open_fds(prometheus_process_provider_t provider);
prometheus_metric_t prometheus_process_provider_max_fds(prometheus_process_provider_t provider);
prometheus_metric_t prometheus_process_provider_virtual_memory_max_bytes(prometheus_process_provider_t provider);

prometheus_collector_t
prometheus_process_collector_create(prometheus_process_provider_t provider, const char * name);

#endif
