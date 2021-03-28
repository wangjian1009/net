#ifndef PROMETHEUS_HTTP_PROCESSOR_H_INCLEDED
#define PROMETHEUS_HTTP_PROCESSOR_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "prometheus_http_types.h"

prometheus_http_processor_t
prometheus_http_processor_create(
    prometheus_manager_t manager,
    error_monitor_t em, mem_allocrator_t alloc);

void prometheus_process_provider_free(prometheus_http_processor_t processor);

#endif
