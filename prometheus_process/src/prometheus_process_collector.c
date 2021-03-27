#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "prometheus_collector.h"
#include "prometheus_collector_metric.h"
#include "prometheus_gauge.h"
#include "prometheus_process_collector_i.h"
#include "prometheus_process_limits_i.h"
#include "prometheus_process_fds_i.h"

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

/*limit*/
struct prometheus_process_collector_limits_ctx {
    prometheus_process_collector_t m_collector;
    prometheus_collector_metric_t m_max_fds;
    prometheus_collector_metric_t m_virtual_memory_max_bytes;
};

void prometheus_process_collector_on_row(void * i_ctx, prometheus_process_limits_row_t row) {
    struct prometheus_process_collector_limits_ctx * ctx = i_ctx;
    prometheus_process_provider_t provider = ctx->m_collector->m_provider;

    if (strcmp(row->limit, "Max open files") == 0) {
        if (ctx->m_max_fds != NULL) {
            prometheus_gauge_t guage = prometheus_gauge_cast(prometheus_collector_metric_metric(ctx->m_max_fds));
            if (prometheus_gauge_set(guage, row->soft, NULL) != 0) {
                CPE_ERROR(
                    provider->m_em, "prometheus: process: collect: limit.%s: set gauge failed", row->limit);
                return;
            }

            prometheus_collector_metric_set_state(ctx->m_max_fds, prometheus_metric_collected);
        }
    }
    else if (strcmp(row->limit, "Max address space") == 0) {
    }
    
/*   // Retrieve the *prom_process_limits_row_t for Max address space */
/*   prom_process_limits_row_t *virtual_memory_max_bytes = */
/*       (prom_process_limits_row_t *)prom_map_get(limits_map, "Max address space"); */
/*   if (virtual_memory_max_bytes == NULL) { */
/*     prom_process_limits_file_destroy(limits_f); */
/*     prom_map_destroy(limits_map); */
/*     return NULL; */
/*   } */

    
}

void prometheus_process_collector_from_limits(prometheus_collector_t base_collector) {
    prometheus_process_collector_t collector = prometheus_collector_data(base_collector);
    prometheus_process_provider_t provider = collector->m_provider;
    assert(provider);

    struct prometheus_process_collector_limits_ctx limit_ctx = {
        collector,
        prometheus_collector_metric_find_by_metric(
            base_collector, prometheus_process_provider_max_fds(provider)),
        prometheus_collector_metric_find_by_metric(
            base_collector, prometheus_process_provider_virtual_memory_max_bytes(provider)),
    };

    if (limit_ctx.m_max_fds == NULL
        && limit_ctx.m_virtual_memory_max_bytes == NULL
        )
    {
        return;
    }

    if (prometheus_process_limits_load(provider, &limit_ctx, prometheus_process_collector_on_row) != 0) {
        CPE_ERROR(provider->m_em, "prometheus: process: collect: process limit fail");
    }
}

void prometheus_process_collector_from_state(prometheus_collector_t base_collector) {
    prometheus_process_collector_t collector = prometheus_collector_data(base_collector);
    prometheus_process_provider_t provider = collector->m_provider;
    assert(provider);

    prometheus_collector_metric_t cpu_seconds_total =
        prometheus_collector_metric_find_by_metric(
            base_collector, prometheus_process_provider_cpu_seconds_total(provider));

    prometheus_collector_metric_t virtual_memory_bytes =
        prometheus_collector_metric_find_by_metric(
            base_collector, prometheus_process_provider_virtual_memory_bytes(provider));
    
    prometheus_collector_metric_t resident_memory_bytes =
        prometheus_collector_metric_find_by_metric(
            base_collector, prometheus_process_provider_resident_memory_bytes(provider));
    
    /* prometheus_gauge_t m_cpu_seconds_total; */
    /* prometheus_gauge_t m_virtual_memory_bytes; */
    /* prometheus_gauge_t m_resident_memory_bytes; */
    /* prometheus_gauge_t m_start_time_seconds; */
    
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
}

void prometheus_process_collector_open_fds(prometheus_collector_t base_collector) {
    prometheus_process_collector_t collector = prometheus_collector_data(base_collector);
    prometheus_process_provider_t provider = collector->m_provider;
    assert(provider);

    prometheus_collector_metric_t open_fds =
        prometheus_collector_metric_find_by_metric(
            base_collector, prometheus_process_provider_open_fds(provider));
    if (open_fds == NULL) return;

    
}

void prometheus_process_collector_collect(prometheus_collector_t base_collector) {
    prometheus_process_collector_from_limits(base_collector);
    prometheus_process_collector_from_state(base_collector);
    prometheus_process_collector_open_fds(base_collector);
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
    if (base_collector == NULL) {
        CPE_ERROR(provider->m_em, "prometheus: process: create collector %s fail!", name);
        return NULL;
    }
    
    prometheus_process_collector_t collector = prometheus_collector_data(base_collector);
    collector->m_provider = provider;
    TAILQ_INSERT_TAIL(&provider->m_collectors, collector, m_next);
    
    return base_collector;
}

void prometheus_process_collector_free(prometheus_process_collector_t collector) {
    prometheus_collector_free(prometheus_collector_from_data(collector));
}
