#include "cpe/utils/string_utils.h"
#include "prometheus_collector_i.h"
#include "prometheus_collector_metric_i.h"

prometheus_collector_t
prometheus_collector_create(prometheus_manager_t manager, const char * name) {
    prometheus_collector_t collector = mem_alloc(manager->m_alloc, sizeof(struct prometheus_collector));
    if (collector == NULL) {
        CPE_ERROR(manager->m_em, "prometheus: collector %s: alloc fail", name);
        return NULL;
    }

    collector->m_manager = manager;
    collector->m_name = cpe_str_mem_dup(manager->m_alloc, name);
    if (collector->m_name == NULL) {
        CPE_ERROR(manager->m_em, "prometheus: collector %s: dup name fail", name);
        mem_free(manager->m_alloc, collector);
        return NULL;
    }
    
    TAILQ_INIT(&collector->m_metrics);

    TAILQ_INSERT_TAIL(&manager->m_collectors, collector, m_next);
    return collector;
}

void prometheus_collector_free(prometheus_collector_t collector) {
    prometheus_manager_t manager = collector->m_manager;

    if (collector->m_name) {
        mem_free(manager->m_alloc, collector->m_name);
        collector->m_name = NULL;
    }

    while(!TAILQ_EMPTY(&collector->m_metrics)) {
        prometheus_collector_metric_free(TAILQ_FIRST(&collector->m_metrics));
    }

    mem_free(manager->m_alloc, collector);
}

int prometheus_collector_add_metric(prometheus_collector_t collector, prometheus_metric_t metric) {
    return 0;
}



