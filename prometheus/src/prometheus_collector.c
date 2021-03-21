#include "cpe/utils/string_utils.h"
#include "prometheus_collector_i.h"
#include "prometheus_collector_metric_i.h"

prometheus_collector_t
prometheus_collector_create(
    prometheus_manager_t manager, const char * name,
    uint32_t capacity,
    prometheus_collector_init_fun_t init,
    prometheus_collector_fini_fun_t fini,
    prometheus_collector_collect_fun_t collect)
{
    prometheus_collector_t collector =
        mem_alloc(manager->m_alloc, sizeof(struct prometheus_collector) + capacity);
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
    collector->m_fini = fini;
    collector->m_collect = collect;
        
    TAILQ_INIT(&collector->m_metrics);

    if (init && !init(collector)) {
        CPE_ERROR(manager->m_em, "prometheus: collector %s: init fail", name);
        if (collector->m_name) mem_free(manager->m_alloc, collector->m_name);
        mem_free(manager->m_alloc, collector);
        return NULL;
    }
    
    TAILQ_INSERT_TAIL(&manager->m_collectors, collector, m_next);
    return collector;
}

void prometheus_collector_free(prometheus_collector_t collector) {
    prometheus_manager_t manager = collector->m_manager;

    if (collector->m_fini) {
        collector->m_fini(collector);
        collector->m_fini = NULL;
    }
    
    if (collector->m_name) {
        mem_free(manager->m_alloc, collector->m_name);
        collector->m_name = NULL;
    }

    while(!TAILQ_EMPTY(&collector->m_metrics)) {
        prometheus_collector_metric_free(TAILQ_FIRST(&collector->m_metrics));
    }

    if (manager->m_collector_default == collector) {
        manager->m_collector_default = NULL;
    }

    TAILQ_REMOVE(&manager->m_collectors, collector, m_next);
    mem_free(manager->m_alloc, collector);
}

int prometheus_collector_add_metric(prometheus_collector_t collector, prometheus_metric_t metric) {
    prometheus_collector_metric_t collector_metric
        = prometheus_collector_metric_create(metric, collector);
    if (collector_metric == NULL) {
        return -1;
    }

    return 0;
}

prometheus_collector_t prometheus_collector_default(prometheus_manager_t manager) {
    return manager->m_collector_default;
}

void * prometheus_collector_data(prometheus_collector_t collector) {
    return collector + 1;
}

prometheus_collector_t prometheus_collector_from_data(void * data) {
    return ((prometheus_collector_t)data) - 1;
}
