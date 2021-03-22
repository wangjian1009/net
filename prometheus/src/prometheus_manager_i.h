#ifndef PROMETHEUS_MANAGER_I_H_INCLEDED
#define PROMETHEUS_MANAGER_I_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/error.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/hash.h"
#include "cpe/utils/buffer.h"
#include "prometheus_manager.h"

typedef struct prometheus_metric_type * prometheus_metric_type_t;
typedef struct prometheus_metric_formatter * prometheus_metric_formatter_t;
typedef struct prometheus_string_builder * prometheus_string_builder_t;

typedef TAILQ_HEAD(prometheus_metric_list, prometheus_metric) prometheus_metric_list_t;
typedef TAILQ_HEAD(prometheus_metric_sample_list, prometheus_metric_sample) prometheus_metric_sample_list_t;
typedef TAILQ_HEAD(prometheus_collector_list, prometheus_collector) prometheus_collector_list_t;
typedef TAILQ_HEAD(prometheus_collector_metric_list, prometheus_collector_metric) prometheus_collector_metric_list_t;
typedef TAILQ_HEAD(prometheus_histogram_buckets_list, prometheus_histogram_buckets) prometheus_histogram_buckets_list_t;

struct prometheus_manager {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;

    /*metric type*/
    prometheus_metric_type_t m_metric_types[4];

    /*metric*/
    struct cpe_hash_table m_metrics;

    /*collector*/
    prometheus_collector_t m_collector_default;
    prometheus_collector_list_t m_collectors;

    /**/
    prometheus_histogram_buckets_t m_histogram_default_buckets;
    prometheus_histogram_buckets_list_t m_histogram_bucketses;
    
    /**/
    struct mem_buffer m_tmp_buffer;
};
    
#endif
