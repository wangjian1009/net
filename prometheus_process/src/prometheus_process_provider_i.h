#ifndef PROMETHEUS_PROCESS_PROVIDER_I_H_INCLEDED
#define PROMETHEUS_PROCESS_PROVIDER_I_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "cpe/utils/buffer.h"
#include "prometheus_process_provider.h"

typedef struct prometheus_process_linux_stat * prometheus_process_linux_stat_t;

typedef struct prometheus_process_linux_limits_row * prometheus_process_linux_limits_row_t;
typedef struct prometheus_process_linux_limits_parse_ctx * prometheus_process_linux_limits_parse_ctx_t;
    
typedef struct prometheus_process_collector * prometheus_process_collector_t;
typedef TAILQ_HEAD(prometheus_process_collector_list, prometheus_process_collector) prometheus_process_collector_list_t;

struct prometheus_process_provider {
    error_monitor_t m_em;
    mem_allocrator_t m_alloc;
    vfs_mgr_t m_vfs_mgr;
    prometheus_manager_t m_manager;
    char * m_limits_path;
    char * m_stat_path;

    prometheus_gauge_t m_cpu_seconds_total;
    prometheus_gauge_t m_virtual_memory_bytes;
    prometheus_gauge_t m_resident_memory_bytes;
    prometheus_gauge_t m_start_time_seconds;
    
    prometheus_gauge_t m_open_fds;

    prometheus_gauge_t m_max_fds;
    prometheus_gauge_t m_virtual_memory_max_bytes;

    prometheus_process_collector_list_t m_collectors;
    struct mem_buffer m_data_buffer;
};

#endif
