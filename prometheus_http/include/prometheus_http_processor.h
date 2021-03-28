#ifndef PROMETHEUS_HTTP_PROCESSOR_H_INCLEDED
#define PROMETHEUS_HTTP_PROCESSOR_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_http_svr_system.h"
#include "prometheus_http_types.h"

prometheus_http_processor_t
prometheus_http_processor_create(
    net_http_svr_protocol_t http_svr,
    prometheus_manager_t manager,
    error_monitor_t em, mem_allocrator_t alloc);

void prometheus_process_provider_free(prometheus_http_processor_t processor);

#endif
