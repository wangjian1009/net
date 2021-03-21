#ifndef PROMETHEUS_COLLECTOR_I_H_INCLEDED
#define PROMETHEUS_COLLECTOR_I_H_INCLEDED
#include "prometheus_collector.h"
#include "prometheus_manager_i.h"

struct prometheus_collector {
    prometheus_manager_t m_manager;
    TAILQ_ENTRY(prometheus_collector) m_next;
    char * m_name;
    prometheus_collector_metric_list_t m_metrics;
    //prom_collect_fn * collect_fn;
};

#endif
