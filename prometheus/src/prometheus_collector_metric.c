#include "cpe/pal/pal_string.h"
#include "prometheus_collector_metric_i.h"
#include "prometheus_metric_i.h"
#include "prometheus_collector_i.h"

prometheus_collector_metric_t
prometheus_collector_metric_create(prometheus_metric_t metric, prometheus_collector_t collector) {
    prometheus_manager_t manager = metric->m_manager;

    prometheus_collector_metric_t collector_metric = mem_alloc(manager->m_alloc, sizeof(struct prometheus_collector_metric));
    if (collector_metric == NULL) {
        CPE_ERROR(manager->m_em, "prometheus: %s: collector_metric to %s", metric->m_name, collector->m_name);
        return NULL;
    }

    collector_metric->m_metric = metric;
    collector_metric->m_collector = collector;

    TAILQ_INSERT_TAIL(&metric->m_collectors, collector_metric, m_next_for_metric);
    TAILQ_INSERT_TAIL(&collector->m_metrics, collector_metric, m_next_for_collector);
    
    return collector_metric;
}

void prometheus_collector_metric_free(prometheus_collector_metric_t collector_metric) {
    prometheus_manager_t manager = collector_metric->m_metric->m_manager;

    TAILQ_REMOVE(&collector_metric->m_metric->m_collectors, collector_metric, m_next_for_metric);
    TAILQ_REMOVE(&collector_metric->m_collector->m_metrics, collector_metric, m_next_for_collector);

    mem_free(manager->m_alloc, collector_metric);
}

prometheus_collector_metric_t
prometheus_collector_metric_find(prometheus_collector_t collector, const char * name) {
    prometheus_collector_metric_t collector_metric;

    TAILQ_FOREACH(collector_metric, &collector->m_metrics, m_next_for_collector) {
        if (strcmp(collector_metric->m_metric->m_name, name) == 0) return collector_metric;
    }

    return NULL;
}

prometheus_collector_metric_t
prometheus_collector_metric_find_by_metric(
    prometheus_collector_t collector, prometheus_metric_t metric)
{
    prometheus_collector_metric_t collector_metric;

    TAILQ_FOREACH(collector_metric, &collector->m_metrics, m_next_for_collector) {
        if (collector_metric->m_metric == metric) return collector_metric;
    }

    return NULL;
}

prometheus_collector_t
prometheus_collector_metric_collector(prometheus_collector_metric_t collector_metric) {
    return collector_metric->m_collector;
}

prometheus_metric_t
prometheus_collector_metric_metric(prometheus_collector_metric_t collector_metric) {
    return collector_metric->m_metric;
}

prometheus_metric_collect_state_t
prometheus_collector_metric_state(prometheus_collector_metric_t collector_metric) {
    return collector_metric->m_state;
}

void prometheus_collector_metric_set_state(
    prometheus_collector_metric_t collector_metric, prometheus_metric_collect_state_t state)
{
    collector_metric->m_state = state;
}

const char * prometheus_metric_collect_state_str(prometheus_metric_collect_state_t state) {
    switch(state) {
    case prometheus_metric_not_collect:
        return "not-collect";
    case prometheus_metric_collected:
        return "collected";
    }
}
