#include "cpe/pal/pal_unistd.h"
#include "cpe/utils/string_utils.h"
#include "prometheus_gauge.h"
#include "prometheus_metric.h"
#include "prometheus_collector.h"
#include "prometheus_process_provider_i.h"
#include "prometheus_process_collector_i.h"
#include "prometheus_process_stat_i.h"
#include "prometheus_process_fds_i.h"
#include "prometheus_process_limits_i.h"

prometheus_process_provider_t
prometheus_process_provider_create(
    prometheus_manager_t manager, error_monitor_t em, mem_allocrator_t alloc,
    const char * limits_path, const char *stat_path)
{
    prometheus_process_provider_t provider = mem_alloc(alloc, sizeof(struct prometheus_process_provider));
    if (provider == NULL) {
        CPE_ERROR(em, "prometheus: process: alloc provier fail!");
        return NULL;
    }

    provider->m_alloc = alloc;
    provider->m_em = em;
    provider->m_manager = manager;
    provider->m_limits_path = NULL;
    provider->m_stat_path = NULL;

    provider->m_cpu_seconds_total = NULL;
    provider->m_virtual_memory_bytes = NULL;
    provider->m_resident_memory_bytes = NULL;
    provider->m_start_time_seconds = NULL;
    
    provider->m_open_fds = NULL;

    provider->m_max_fds = NULL;
    provider->m_virtual_memory_max_bytes = NULL;

    TAILQ_INIT(&provider->m_collectors);

    /*初始化完成，后续可以free */

    int pid = (int)getpid();
    char path_buf[50];

    /*lmits path*/
    if (limits_path == NULL) {
        snprintf(path_buf, sizeof(path_buf), "/proc/%d/limits", pid);
        limits_path = path_buf;
    }
    provider->m_limits_path = cpe_str_mem_dup(provider->m_alloc, limits_path);
    if (provider->m_limits_path == NULL) {
        CPE_ERROR(provider->m_em, "prometheus: process: dup limits path %s fail!", limits_path);
        goto INIT_ERROR;
    }

    /*stat path*/
    if (stat_path == NULL) {
        snprintf(path_buf, sizeof(path_buf), "/proc/%d/stat", pid);
        stat_path = path_buf;
    }
    provider->m_stat_path = cpe_str_mem_dup(provider->m_alloc, stat_path);
    if (provider->m_stat_path == NULL) {
        CPE_ERROR(provider->m_em, "prometheus: process: dup stat path %s fail!", stat_path);
        goto INIT_ERROR;
    }

    /*metric*/
    provider->m_open_fds =
        prometheus_gauge_create(
            manager,
            "process_open_fds",
            "Number of open file descriptors.", 0, NULL);
    if (provider->m_open_fds == NULL) {
        CPE_ERROR(provider->m_em, "prometheus: process: create metric process_open_fds fail!");
        goto INIT_ERROR;
    }
        
    provider->m_max_fds =
        prometheus_gauge_create(
            provider->m_manager,
            "process_max_fds",
            "Maximum number of open file descriptors.", 0, NULL);
    if (provider->m_open_fds == NULL) {
        CPE_ERROR(provider->m_em, "prometheus: process: create metric process_max_fds fail!");
        goto INIT_ERROR;
    }

    provider->m_virtual_memory_max_bytes =
        prometheus_gauge_create(
            provider->m_manager,
            "process_virtual_memory_max_bytes",
            "Maximum amount of virtual memory available in bytes.", 0, NULL);
    if (provider->m_virtual_memory_max_bytes == NULL) {
        CPE_ERROR(provider->m_em, "prometheus: process: create metric process_virtual_memory_max_bytes fail!");
        goto INIT_ERROR;
    }

    // /proc/[pid]stat cutime + cstime / 100
    provider->m_cpu_seconds_total =
        prometheus_gauge_create(
            provider->m_manager,
            "process_cpu_seconds_total",
            "Total user and system CPU time spent in seconds.", 0, NULL);
    if (provider->m_cpu_seconds_total == NULL) {
        CPE_ERROR(provider->m_em, "prometheus: process: create metric process_cpu_seconds_total fail!");
        goto INIT_ERROR;
    }

    // /proc/[pid]/stat Field 23
    provider->m_virtual_memory_bytes =
        prometheus_gauge_create(
            provider->m_manager,
            "process_virtual_memory_bytes",
            "Virtual memory size in bytes.", 0, NULL);
    if (provider->m_virtual_memory_bytes) {
        CPE_ERROR(provider->m_em, "prometheus: process: create metric process_virtual_memory_bytes fail!");
        goto INIT_ERROR;
    }

    // /proc/[pid]/stat Field 24
    provider->m_resident_memory_bytes =
        prometheus_gauge_create(
            provider->m_manager,
            "process_resident_memory_bytes",
            "Resident memory size in bytes.", 0, NULL);
    if (provider->m_resident_memory_bytes == NULL) {
        CPE_ERROR(provider->m_em, "prometheus: process: create metric process_resident_memory_bytes fail!");
        goto INIT_ERROR;
    }
    
    provider->m_start_time_seconds =
        prometheus_gauge_create(
            provider->m_manager,
            "process_start_time_seconds",
            "Start time of the process since unix epoch in seconds.", 0, NULL);
    if (provider->m_start_time_seconds == NULL) {
        CPE_ERROR(provider->m_em, "prometheus: process: create metric process_start_time_seconds fail!");
        goto INIT_ERROR;
    }

    return provider;

INIT_ERROR:
    prometheus_process_provider_free(provider);
    return NULL;
}

void prometheus_process_provider_free(prometheus_process_provider_t provider) {
    if (provider->m_cpu_seconds_total) {
        prometheus_gauge_free(provider->m_cpu_seconds_total);
        provider->m_cpu_seconds_total = NULL;
    }

    if (provider->m_virtual_memory_bytes) {
        prometheus_gauge_free(provider->m_virtual_memory_bytes);
        provider->m_virtual_memory_bytes = NULL;
    }
    
    /* prometheus_gauge_t m_resident_memory_bytes; */
    /* prometheus_gauge_t m_start_time_seconds; */
    
    /* prometheus_gauge_t m_open_fds; */

    /* prometheus_gauge_t m_max_fds; */
    /* prometheus_gauge_t m_virtual_memory_max_bytes; */

    while(!TAILQ_EMPTY(&provider->m_collectors)) {
        prometheus_process_collector_free(TAILQ_FIRST(&provider->m_collectors));
    }

    mem_free(provider->m_alloc, provider);
}

/* prometheus_collector_t */
/* prometheus_process_collector_create(prometheus_manager_t manager, const char * name) { */
    /* provider->m_collector = prometheus_collector_create(provider->m_manager, "process"); */
    /* if (provider->m_collector == NULL) { */
    /*     CPE_ERROR(provider->m_em, "prometheus: process: create: create collector fail!"); */
    /*     goto INIT_ERROR; */
    /* } */

    /* if (prometheus_collector_add_metric(provider->m_collector, prometheus_metric_from_data(provider->m_max_fds)) != 0 */
    /*     || prometheus_collector_add_metric(provider->m_collector, prometheus_metric_from_data(provider->m_virtual_memory_max_bytes)) != 0 */
    /*     || prometheus_collector_add_metric(provider->m_collector, prometheus_metric_from_data(provider->m_cpu_seconds_total)) != 0 */
    /*     || prometheus_collector_add_metric(provider->m_collector, prometheus_metric_from_data(provider->m_virtual_memory_bytes)) != 0 */
    /*     || prometheus_collector_add_metric(provider->m_collector, prometheus_metric_from_data(provider->m_resident_memory_bytes)) != 0 */
    /*     || prometheus_collector_add_metric(provider->m_collector, prometheus_metric_from_data(provider->m_start_time_seconds)) != 0 */
    /*     || prometheus_collector_add_metric(provider->m_collector, prometheus_metric_from_data(provider->m_open_fds)) != 0 */
    /*     ) */
    /* { */
    /*     CPE_ERROR(provider->m_em, "prometheus: process: create: collector add metric!"); */
    /*     goto INIT_ERROR; */
    /* } */
/* } */

/* prom_map_t *prom_collector_process_collect(prom_collector_t *self); */

/* prom_map_t *prom_collector_process_collect(prom_collector_t *self) { */
/*   PROM_ASSERT(self != NULL); */
/*   if (self == NULL) return NULL; */

/*   int r = 0; */

/*   // Allocate and create a *prom_process_limits_file_t */
/*   prom_process_limits_file_t *limits_f = prom_process_limits_file_new(self->proc_limits_file_path); */
/*   if (limits_f == NULL) { */
/*     prom_process_limits_file_destroy(limits_f); */
/*     return NULL; */
/*   } */

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
/* } */
