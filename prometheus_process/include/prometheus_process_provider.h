#ifndef PROMETHEUS_PROCESS_COLLECTOR_H_INCLEDED
#define PROMETHEUS_PROCESS_COLLECTOR_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "prometheus_process_types.h"

prometheus_process_provider_t
prometheus_process_provider_create(
    prometheus_manager_t manager,
    error_monitor_t em, mem_allocrator_t alloc,
    const char * limits_path, const char *stat_path);

void prometheus_process_provider_free(prometheus_process_provider_t process_provider);

#endif
