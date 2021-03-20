#ifndef PROMETHEUS_METRIC_SAMPLE_I_H_INCLEDED
#define PROMETHEUS_METRIC_SAMPLE_I_H_INCLEDED
#include "prometheus_metric_sample.h"
#include "prometheus_metric_i.h"

struct prometheus_metric_sample {
    prometheus_metric_t m_metric;
    cpe_hash_entry m_hh_for_metric;
    char * m_l_value;
    double m_r_value;
};

prometheus_metric_sample_t
prometheus_metric_sample_create(
    prometheus_metric_t metric, const char * l_value, double r_value);

void prometheus_metric_sample_free(prometheus_metric_sample_t sample);

void prometheus_metric_sample_free_all(prometheus_metric_t metric);

uint32_t prometheus_metric_sample_hash(prometheus_metric_sample_t sample, void * user_data);
int prometheus_metric_sample_eq(prometheus_metric_sample_t l, prometheus_metric_sample_t r);

#endif
