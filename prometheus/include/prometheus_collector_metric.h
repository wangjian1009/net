#ifndef PROMETHEUS_COLLECTOR_METRIC_H_INCLEDED
#define PROMETHEUS_COLLECTOR_METRIC_H_INCLEDED
#include "prometheus_types.h"

CPE_BEGIN_DECL

enum prometheus_metric_collect_state {
    prometheus_metric_not_collect,
    prometheus_metric_collected,
};

prometheus_collector_metric_t
prometheus_collector_metric_find(prometheus_collector_t collector, const char * name);

prometheus_collector_t
prometheus_collector_metric_collector(prometheus_collector_metric_t metric);

prometheus_metric_t
prometheus_collector_metric_metric(prometheus_collector_metric_t metric);

prometheus_metric_collect_state_t
prometheus_collector_metric_state(prometheus_collector_metric_t metric);

void prometheus_collector_metric_set_state(
    prometheus_collector_metric_t metric, prometheus_metric_collect_state_t state);

const char * prometheus_metric_collect_state_str(prometheus_metric_collect_state_t state);

CPE_END_DECL

#endif
