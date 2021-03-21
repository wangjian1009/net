#ifndef PROMETHEUS_COLLECTOR_H_INCLEDED
#define PROMETHEUS_COLLECTOR_H_INCLEDED
#include "prometheus_types.h"

CPE_BEGIN_DECL

typedef int (*prometheus_collector_init_fun_t)(prometheus_collector_t collector);
typedef void (*prometheus_collector_fini_fun_t)(prometheus_collector_t collector);
typedef void (*prometheus_collector_collect_fun_t)(prometheus_collector_t collector);

prometheus_collector_t
prometheus_collector_create(
    prometheus_manager_t manager, const char * name,
    uint32_t capacity,
    prometheus_collector_init_fun_t init,
    prometheus_collector_fini_fun_t fini,
    prometheus_collector_collect_fun_t collect);

void prometheus_collector_free(prometheus_collector_t collector);

int prometheus_collector_add_metric(prometheus_collector_t collector, prometheus_metric_t metric);

prometheus_collector_t prometheus_collector_default(prometheus_manager_t manager);

void * prometheus_collector_data(prometheus_collector_t collector);
prometheus_collector_t prometheus_collector_from_data(void * data);

CPE_END_DECL

#endif
