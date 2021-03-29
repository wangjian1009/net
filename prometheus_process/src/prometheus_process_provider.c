#include "cpe/pal/pal_unistd.h"
#include "cpe/utils/string_utils.h"
#include "prometheus_gauge.h"
#include "prometheus_counter.h"
#include "prometheus_metric.h"
#include "prometheus_collector.h"
#include "prometheus_process_provider_i.h"
#include "prometheus_process_collector_i.h"
#include "prometheus_process_linux_stat_i.h"
#include "prometheus_process_linux_fds_i.h"
#include "prometheus_process_linux_limits_i.h"

prometheus_process_provider_t
prometheus_process_provider_create(
    prometheus_manager_t manager,
    error_monitor_t em, mem_allocrator_t alloc, vfs_mgr_t vfs)
{
    prometheus_process_provider_t provider = mem_alloc(alloc, sizeof(struct prometheus_process_provider));
    if (provider == NULL) {
        CPE_ERROR(em, "prometheus: process: alloc provier fail!");
        return NULL;
    }

    provider->m_alloc = alloc;
    provider->m_em = em;
    provider->m_vfs_mgr = vfs;
    provider->m_manager = manager;

    provider->m_cpu_seconds_total = NULL;
    provider->m_virtual_memory_bytes = NULL;
    provider->m_resident_memory_bytes = NULL;
    provider->m_start_time_seconds = NULL;
    
    provider->m_open_fds = NULL;

    provider->m_max_fds = NULL;
    provider->m_virtual_memory_max_bytes = NULL;

    TAILQ_INIT(&provider->m_collectors);
    mem_buffer_init(&provider->m_data_buffer, provider->m_alloc);

    /*初始化完成，后续可以free */

    /*metric*/
    /* /proc/[pid]stat cutime + cstime / 100 */
    provider->m_cpu_seconds_total =
        prometheus_counter_create(
            provider->m_manager,
            "process_cpu_seconds_total",
            "Total user and system CPU time spent in seconds.", 0, NULL);
    if (provider->m_cpu_seconds_total == NULL) {
        CPE_ERROR(provider->m_em, "prometheus: process: create metric process_cpu_seconds_total fail!");
        goto INIT_ERROR;
    }

    /* /proc/[pid]/stat Field 23 */
    provider->m_virtual_memory_bytes =
        prometheus_gauge_create(
            provider->m_manager,
            "process_virtual_memory_bytes",
            "Virtual memory size in bytes.", 0, NULL);
    if (provider->m_virtual_memory_bytes == NULL) {
        CPE_ERROR(provider->m_em, "prometheus: process: create metric process_virtual_memory_bytes fail!");
        goto INIT_ERROR;
    }

    /* /proc/[pid]/stat Field 24 */
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

    provider->m_open_fds =
        prometheus_gauge_create(
            manager,
            "process_open_fds",
            "Number of open file descriptors.", 0, NULL);
    if (provider->m_open_fds == NULL) {
        CPE_ERROR(provider->m_em, "prometheus: process: create metric process_open_fds fail!");
        goto INIT_ERROR;
    }

    /*limits*/
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

    return provider;

INIT_ERROR:
    prometheus_process_provider_free(provider);
    return NULL;
}

void prometheus_process_provider_free(prometheus_process_provider_t provider) {
    while(!TAILQ_EMPTY(&provider->m_collectors)) {
        prometheus_process_collector_free(TAILQ_FIRST(&provider->m_collectors));
    }
    
    if (provider->m_cpu_seconds_total) {
        prometheus_counter_free(provider->m_cpu_seconds_total);
        provider->m_cpu_seconds_total = NULL;
    }

    if (provider->m_virtual_memory_bytes) {
        prometheus_gauge_free(provider->m_virtual_memory_bytes);
        provider->m_virtual_memory_bytes = NULL;
    }
    
    if (provider->m_resident_memory_bytes) {
        prometheus_gauge_free(provider->m_resident_memory_bytes);
        provider->m_resident_memory_bytes = NULL;
    }
    
    if (provider->m_start_time_seconds) {
        prometheus_gauge_free(provider->m_start_time_seconds);
        provider->m_start_time_seconds = NULL;
    }
    
    if (provider->m_open_fds) {
        prometheus_gauge_free(provider->m_open_fds);
        provider->m_open_fds = NULL;
    }

    if (provider->m_max_fds) {
        prometheus_gauge_free(provider->m_max_fds);
        provider->m_max_fds = NULL;
    }
    
    if (provider->m_virtual_memory_max_bytes) {
        prometheus_gauge_free(provider->m_virtual_memory_max_bytes);
        provider->m_virtual_memory_max_bytes = NULL;
    }

    mem_buffer_clear(&provider->m_data_buffer);

    mem_free(provider->m_alloc, provider);
}

prometheus_metric_t
prometheus_process_provider_cpu_seconds_total(prometheus_process_provider_t provider) {
    return prometheus_metric_from_data(provider->m_cpu_seconds_total);
}

prometheus_metric_t
prometheus_process_provider_virtual_memory_bytes(prometheus_process_provider_t provider){
    return prometheus_metric_from_data(provider->m_virtual_memory_bytes);
}
        
prometheus_metric_t
prometheus_process_provider_resident_memory_bytes(prometheus_process_provider_t provider) {
    return prometheus_metric_from_data(provider->m_resident_memory_bytes);
}

prometheus_metric_t
prometheus_process_provider_start_time_seconds(prometheus_process_provider_t provider) {
    return prometheus_metric_from_data(provider->m_start_time_seconds);
}

prometheus_metric_t
prometheus_process_provider_open_fds(prometheus_process_provider_t provider) {
    return prometheus_metric_from_data(provider->m_open_fds);
}

prometheus_metric_t
prometheus_process_provider_max_fds(prometheus_process_provider_t provider) {
    return prometheus_metric_from_data(provider->m_max_fds);
}

prometheus_metric_t
prometheus_process_provider_virtual_memory_max_bytes(prometheus_process_provider_t provider) {
    return prometheus_metric_from_data(provider->m_virtual_memory_max_bytes);
}
