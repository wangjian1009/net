#include <assert.h>
#include "prometheus_collector.h"
#include "prometheus_process_collector_i.h"
#include "prometheus_process_limits_i.h"

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

void prometheus_process_collector_on_row(void * ctx, prometheus_process_limits_row_t row) {
    prometheus_process_collector_t collector = ctx;
}

void prometheus_process_collector_collect(prometheus_collector_t base_collector) {
    prometheus_process_collector_t collector = prometheus_collector_data(base_collector);
    prometheus_process_provider_t provider = collector->m_provider;
    assert(provider);
    
    if (prometheus_process_limits_load(provider, provider, prometheus_process_collector_on_row) != 0) {
        CPE_ERROR(provider->m_em, "prometheus: process: collect: process limit fail");
    }

/*   // Allocate and create a *prom_map_t from prom_process_limits_file_t. This is the main storage container for the */
/*   // limits metric data */
/*   prom_map_t *limits_map = prom_process_limits(limits_f); */
/*   if (limits_map == NULL) { */
/*     prom_process_limits_file_destroy(limits_f); */
/*     prom_map_destroy(limits_map); */
/*     return NULL; */
/*   } */

/*   // Retrieve the *prom_process_limits_row_t for Max open files */
/*   prom_process_limits_row_t *max_fds = (prom_process_limits_row_t *)prom_map_get(limits_map, "Max open files"); */
/*   if (max_fds == NULL) { */
/*     prom_process_limits_file_destroy(limits_f); */
/*     prom_map_destroy(limits_map); */
/*     return NULL; */
/*   } */

/*   // Retrieve the *prom_process_limits_row_t for Max address space */
/*   prom_process_limits_row_t *virtual_memory_max_bytes = */
/*       (prom_process_limits_row_t *)prom_map_get(limits_map, "Max address space"); */
/*   if (virtual_memory_max_bytes == NULL) { */
/*     prom_process_limits_file_destroy(limits_f); */
/*     prom_map_destroy(limits_map); */
/*     return NULL; */
/*   } */

/*   // Set the metric values for max_fds and virtual_memory_max_bytes */
/*   r = prom_gauge_set(prom_process_max_fds, max_fds->soft, NULL); */
/*   if (r) return NULL; */
/*   r = prom_gauge_set(prom_process_virtual_memory_max_bytes, virtual_memory_max_bytes->soft, NULL); */
/*   if (r) return NULL; */

/*   // Aloocate and create a *prom_process_stat_file_t */
/*   prom_process_stat_file_t *stat_f = prom_process_stat_file_new(self->proc_stat_file_path); */
/*   if (stat_f == NULL) { */
/*     prom_process_limits_file_destroy(limits_f); */
/*     prom_map_destroy(limits_map); */
/*     return self->metrics; */
/*   } */

/*   // Allocate and create a *prom_process_stat_t from *prom_process_stat_file_t */
/*   prom_process_stat_t *stat = prom_process_stat_new(stat_f); */

/*   // Set the metrics related to the stat file */
/*   r = prom_gauge_set(prom_process_cpu_seconds_total, ((stat->utime + stat->stime) / sysconf(_SC_CLK_TCK)), NULL); */
/*   if (r) { */
/*     prom_process_limits_file_destroy(limits_f); */
/*     prom_map_destroy(limits_map); */
/*     prom_process_stat_file_destroy(stat_f); */
/*     prom_process_stat_destroy(stat); */
/*     return NULL; */
/*   } */
/*   r = prom_gauge_set(prom_process_virtual_memory_bytes, stat->vsize, NULL); */
/*   if (r) { */
/*     prom_process_limits_file_destroy(limits_f); */
/*     prom_map_destroy(limits_map); */
/*     prom_process_stat_file_destroy(stat_f); */
/*     prom_process_stat_destroy(stat); */
/*     return NULL; */
/*   } */
/*   r = prom_gauge_set(prom_process_resident_memory_bytes, stat->rss*sysconf(_SC_PAGE_SIZE), NULL); */
/*   if (r) { */
/*     prom_process_limits_file_destroy(limits_f); */
/*     prom_map_destroy(limits_map); */
/*     prom_process_stat_file_destroy(stat_f); */
/*     prom_process_stat_destroy(stat); */
/*     return NULL; */
/*   } */
/*   r = prom_gauge_set(prom_process_start_time_seconds, stat->starttime, NULL); */
/*   if (r) { */
/*     prom_process_limits_file_destroy(limits_f); */
/*     prom_map_destroy(limits_map); */
/*     prom_process_stat_file_destroy(stat_f); */
/*     prom_process_stat_destroy(stat); */
/*     return NULL; */
/*   } */
/*   r = prom_gauge_set(prom_process_open_fds, prom_process_fds_count(NULL), NULL); */
/*   if (r) { */
/*     prom_process_limits_file_destroy(limits_f); */
/*     prom_map_destroy(limits_map); */
/*     prom_process_stat_file_destroy(stat_f); */
/*     prom_process_stat_destroy(stat); */
/*     return NULL; */
/*   } */

/*   // If there is any issue deallocating the following structures, return NULL to indicate failure */
/*   r = prom_process_limits_file_destroy(limits_f); */
/*   if (r) return NULL; */
/*   r = prom_map_destroy(limits_map); */
/*   if (r) return NULL; */
/*   r = prom_process_stat_file_destroy(stat_f); */
/*   if (r) return NULL; */
/*   r = prom_process_stat_destroy(stat); */
/*   if (r) return NULL; */

/*   return self->metrics; */
}

prometheus_collector_t
prometheus_process_collector_create(prometheus_process_provider_t provider, const char * name) {
    prometheus_collector_t base_collector = 
        prometheus_collector_create(
            provider->m_manager, name,
            sizeof(struct prometheus_process_collector),
            prometheus_process_collector_init,
            prometheus_process_collector_fini,
            prometheus_process_collector_collect);

    prometheus_process_collector_t collector = prometheus_collector_data(base_collector);
    collector->m_provider = provider;
    TAILQ_INSERT_TAIL(&provider->m_collectors, collector, m_next);
    
    return base_collector;
}

void prometheus_process_collector_free(prometheus_process_collector_t collector) {
    prometheus_collector_free(prometheus_collector_from_data(collector));
}
