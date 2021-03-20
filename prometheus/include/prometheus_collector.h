#ifndef PROMETHEUS_COLLECTOR_H_INCLEDED
#define PROMETHEUS_COLLECTOR_H_INCLEDED
#include "prometheus_types.h"

CPE_BEGIN_DECL

typedef void (*prometheus_collector_collect_fun_t)(prometheus_collector_t ctx);

prometheus_collector_t
prometheus_collector_create(prometheus_manager_t manager, const char * name);

void prometheus_collector_free(prometheus_collector_t collector);

int prometheus_collector_add_metric(prometheus_collector_t collector, prometheus_metric_t metric);

CPE_END_DECL

#endif
