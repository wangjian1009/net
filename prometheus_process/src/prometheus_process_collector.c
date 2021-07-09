#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_unistd.h"
#include "prometheus_collector.h"
#include "prometheus_collector_metric.h"
#include "prometheus_metric.h"
#include "prometheus_gauge.h"
#include "prometheus_process_collector_i.h"

int prometheus_process_collector_init(prometheus_collector_t base_collector) {
    prometheus_process_collector_t collector = prometheus_collector_data(base_collector);
    collector->m_provider = NULL;
    return 0;
}

void prometheus_process_collector_fini(prometheus_collector_t base_collector) {
    prometheus_process_collector_t collector = prometheus_collector_data(base_collector);
    prometheus_process_provider_t provider = collector->m_provider;
    if (provider == NULL) return;
    
    TAILQ_REMOVE(&provider->m_collectors, collector, m_next);
}

prometheus_collector_t
prometheus_process_collector_create_i(
    prometheus_process_provider_t provider, const char * name, prometheus_collector_collect_fun_t collect) {
    prometheus_collector_t base_collector = 
        prometheus_collector_create(
            provider->m_manager, name,
            sizeof(struct prometheus_process_collector),
            prometheus_process_collector_init,
            prometheus_process_collector_fini,
            collect);
    if (base_collector == NULL) {
        CPE_ERROR(provider->m_em, "prometheus: process: create collector %s fail!", name);
        return NULL;
    }
    
    prometheus_process_collector_t collector = prometheus_collector_data(base_collector);
    collector->m_provider = provider;
    TAILQ_INSERT_TAIL(&provider->m_collectors, collector, m_next);
    
    return base_collector;
}

void prometheus_process_collector_nooop_collect(prometheus_collector_t base_collector) {
}

prometheus_collector_t
prometheus_process_collector_create(prometheus_process_provider_t provider, const char * name) {
#if CPE_OS_MAC
    return prometheus_process_collector_create_i(provider, name, prometheus_process_collector_mac_collect);
#elif CPE_OS_LINUX
    return prometheus_process_collector_create_i(provider, name, prometheus_process_collector_linux_collect);
#else
    return prometheus_process_collector_create_i(provider, name, prometheus_process_collector_nooop_collect);
#endif
}

prometheus_collector_t
prometheus_process_collector_create_linux(prometheus_process_provider_t provider, const char * name) {
    return prometheus_process_collector_create_i(provider, name, prometheus_process_collector_linux_collect);
}

void prometheus_process_collector_free(prometheus_process_collector_t collector) {
    prometheus_collector_free(prometheus_collector_from_data(collector));
}
