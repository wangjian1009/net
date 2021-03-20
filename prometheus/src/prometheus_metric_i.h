#ifndef PROMETHEUS_METRIC_I_H_INCLEDED
#define PROMETHEUS_METRIC_I_H_INCLEDED
#include "prometheus_metric.h"
#include "prometheus_manager_i.h"

struct prometheus_metric {
    prometheus_manager_t m_manager;
    struct cpe_hash_entry m_hh_for_manager;
    prometheus_metric_type_t m_type;
    const char * m_name;
    const char * m_help;
    struct cpe_hash_table m_samples;
    /* prom_histogram_buckets_t * buckets; /\**< buckets          Array of histogram bucket upper bound values *\/ */
    prometheus_metric_formatter_t m_formatter;
    uint8_t m_label_key_count;
    const char * * m_label_keys;
};

prometheus_metric_t
prometheus_metric_create(
    prometheus_manager_t manager,
    prometheus_metric_category_t category,
    const char *name, const char *help,
    uint8_t label_key_count, const char **label_keys);

void prometheus_metric_print_l_value(
    write_stream_t ws, prometheus_metric_t metric, const char ** label_values, const char * suffix);

char * prometheus_metric_dump_l_value(
    mem_buffer_t buffer, prometheus_metric_t metric, const char ** label_values, const char * suffix);

#endif
