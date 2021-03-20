#ifndef PROMETHEUS_COLLECTOR_I_H_INCLEDED
#define PROMETHEUS_COLLECTOR_I_H_INCLEDED
#include "prometheus_collector.h"
#include "prometheus_manager_i.h"

struct prometheus_collector {
    prometheus_manager_t m_manager;
    const char * name;
    struct cpe_hash_table m_metrics;
    //prom_collect_fn * collect_fn;
    /* prom_string_builder_t * string_builder; */
    /* const char * proc_limits_file_path; */
    /* const char * proc_stat_file_path; */
};

#endif
