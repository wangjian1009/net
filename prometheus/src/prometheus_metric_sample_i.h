#ifndef PROMETHEUS_METRIC_SAMPLE_I_H_INCLEDED
#define PROMETHEUS_METRIC_SAMPLE_I_H_INCLEDED
#include "prometheus_metric_sample.h"
#include "prometheus_metric_i.h"

enum prometheus_metric_sample_owner_type {
    prometheus_metric_sample_owner_metric,
    prometheus_metric_sample_owner_histogram,
};

struct prometheus_metric_sample {
    enum prometheus_metric_sample_owner_type m_owner_type;
    union {
        struct {
            prometheus_metric_t m_metric;
            cpe_hash_entry m_hh;
        } m_owner_metric;
        struct {
            prometheus_metric_sample_histogram_t m_histogram;
            TAILQ_ENTRY(prometheus_metric_sample) m_next;
            double m_upper_bound;
        } m_owner_histogram;
    };
    char * m_l_value;
    double m_r_value;
};

prometheus_metric_sample_t
prometheus_metric_sample_create_for_metric(
    prometheus_metric_t metric, const char * l_value, double r_value);

prometheus_metric_sample_t
prometheus_metric_sample_create_for_histogram(
    prometheus_metric_sample_histogram_t histogram, const char * l_value, double r_value, double upper_bound);

void prometheus_metric_sample_free(prometheus_metric_sample_t sample);

void prometheus_metric_sample_free_all(prometheus_metric_t metric);

uint32_t prometheus_metric_sample_hash(prometheus_metric_sample_t sample, void * user_data);
int prometheus_metric_sample_eq(prometheus_metric_sample_t l, prometheus_metric_sample_t r);

#endif
