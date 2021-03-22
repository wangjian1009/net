#ifndef PROMETHEUS_MANAGER_H_INCLEDED
#define PROMETHEUS_MANAGER_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "prometheus_types.h"

CPE_BEGIN_DECL

prometheus_manager_t
prometheus_manager_create(mem_allocrator_t alloc, error_monitor_t em);

void prometheus_manager_free(prometheus_manager_t manager);

uint8_t prometheus_manager_validate_metric_name(prometheus_manager_t manager, const char *metric_name);

void prometheus_manager_collect(write_stream_t ws, prometheus_manager_t manager);
const char * prometheus_manager_collect_dump(mem_buffer_t buffer, prometheus_manager_t manager);

CPE_END_DECL

#endif
