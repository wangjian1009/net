#ifndef PROMETHEUS_COLLECTOR_METRIC_I_H_INCLEDED
#define PROMETHEUS_COLLECTOR_METRIC_I_H_INCLEDED
#include "prometheus_collector_metric.h"
#include "prometheus_manager_i.h"

struct prometheus_collector_metric {
    prometheus_collector_t m_collector;
    TAILQ_ENTRY(prometheus_collector_metric) m_next_for_collector;
    prometheus_metric_t m_metric;
    TAILQ_ENTRY(prometheus_collector_metric) m_next_for_metric;
    prometheus_metric_collect_state_t m_state;
};

prometheus_collector_metric_t
prometheus_collector_metric_create(prometheus_metric_t metric, prometheus_collector_t collector);

void prometheus_collector_metric_free(prometheus_collector_metric_t bind);

#endif
