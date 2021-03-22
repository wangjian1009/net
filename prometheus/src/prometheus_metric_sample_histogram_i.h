#ifndef PROMETHEUS_METRIC_SAMPLE_HISTOGRAM_I_H
#define PROMETHEUS_METRIC_SAMPLE_HISTOGRAM_I_H
#include "prometheus_metric_sample_histogram.h"
#include "prometheus_metric_i.h"

struct prometheus_metric_sample_histogram {
    prometheus_metric_t m_metric;
    cpe_hash_entry m_hh_for_metric;
    char * m_l_value;
    prometheus_histogram_buckets_t m_buckets;
    prometheus_metric_sample_list_t m_samples_in_order;
    struct cpe_hash_table m_samples;
};

prometheus_metric_sample_histogram_t
prometheus_metric_sample_histogram_create(
    prometheus_metric_t metric, const char * l_value, prometheus_histogram_buckets_t buckets,
    const char **label_values);

void prometheus_metric_sample_histogram_free(prometheus_metric_sample_histogram_t histogram);

void prometheus_metric_sample_histogram_free_all(prometheus_metric_t metric);

uint32_t prometheus_metric_sample_histogram_hash(prometheus_metric_sample_histogram_t sample, void * user_data);
int prometheus_metric_sample_histogram_eq(prometheus_metric_sample_histogram_t l, prometheus_metric_sample_histogram_t r);

char * prometheus_metric_sample_histogram_bucket_to_str(char * buf, size_t buf_capacity, double bucket);

#endif
