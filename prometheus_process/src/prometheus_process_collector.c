#include "prometheus_collector.h"
#include "prometheus_process_collector_i.h"

int prometheus_process_collector_init(prometheus_collector_t collector) {
    return 0;
}

void prometheus_process_collector_fini(prometheus_collector_t collector) {
}

void prometheus_process_collector_collect(prometheus_collector_t collector) {
}

prometheus_collector_t
prometheus_process_collector_create(prometheus_process_provider_t provider, const char * name) {
    prometheus_collector_t collector = 
        prometheus_collector_create(
            provider->m_manager, name,
            sizeof(struct prometheus_process_collector),
            prometheus_process_collector_init,
            prometheus_process_collector_fini,
            prometheus_process_collector_collect);

    prometheus_process_collector_t process_collector = prometheus_collector_data(collector);
    process_collector->m_provider = provider;
    TAILQ_INSERT_TAIL(&provider->m_collectors, process_collector, m_next);
    
    return collector;
}

void prometheus_process_collector_free(prometheus_process_collector_t collector) {
    prometheus_collector_free(prometheus_collector_from_data(collector));
}
