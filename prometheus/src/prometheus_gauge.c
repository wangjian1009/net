#include "prometheus_gauge_i.h"
#include "prometheus_metric_i.h"
#include "prometheus_metric_sample_i.h"
#include "prometheus_metric_type_i.h"

prometheus_gauge_t
prometheus_gauge_create(
    prometheus_manager_t manager,
    const char *name, const char *help,
    size_t label_key_count, const char **label_keys)
{
    prometheus_metric_t metric =
        prometheus_metric_create(
            manager, prometheus_metric_gauge,
            name, help, label_key_count, label_keys);
    if (metric == NULL) return NULL;
    
    prometheus_gauge_t gauge = prometheus_metric_data(metric);
    
    return gauge;
}

void prometheus_gauge_free(prometheus_gauge_t gauge) {
    prometheus_metric_free(prometheus_metric_from_data(gauge));
}

prometheus_gauge_t prometheus_gauge_cast(prometheus_metric_t metric) {
    return metric->m_type->m_category == prometheus_metric_gauge ? (void*)(metric + 1) : NULL;
}

int prometheus_gauge_inc(prometheus_gauge_t gauge, const char ** label_values) {
    prometheus_metric_t metric = prometheus_metric_from_data(gauge);
    prometheus_metric_sample_t sample = prometheus_metric_sample_from_labels(metric, label_values);
    if (sample == NULL) return -1;

    return prometheus_metric_sample_add(sample, 1.0);
}

int prometheus_gauge_dec(prometheus_gauge_t gauge, const char ** label_values) {
    prometheus_metric_t metric = prometheus_metric_from_data(gauge);
    prometheus_metric_sample_t sample = prometheus_metric_sample_from_labels(metric, label_values);
    if (sample == NULL) return -1;

    return prometheus_metric_sample_sub(sample, 1.0);
}

int prometheus_gauge_add(prometheus_gauge_t gauge, double r_value, const char ** label_values) {
    prometheus_metric_t metric = prometheus_metric_from_data(gauge);
    prometheus_metric_sample_t sample = prometheus_metric_sample_from_labels(metric, label_values);
    if (sample == NULL) return -1;

    return prometheus_metric_sample_add(sample, r_value);
}

int prometheus_gauge_sub(prometheus_gauge_t gauge, double r_value, const char ** label_values) {
    prometheus_metric_t metric = prometheus_metric_from_data(gauge);
    prometheus_metric_sample_t sample = prometheus_metric_sample_from_labels(metric, label_values);
    if (sample == NULL) return -1;

    return prometheus_metric_sample_sub(sample, r_value);
}

int prometheus_gauge_set(prometheus_gauge_t gauge, double r_value, const char ** label_values) {
    prometheus_metric_t metric = prometheus_metric_from_data(gauge);
    prometheus_metric_sample_t sample = prometheus_metric_sample_from_labels(metric, label_values);
    if (sample == NULL) return -1;

    return prometheus_metric_sample_set(sample, r_value);
}

prometheus_metric_type_t
prometheus_gauge_type_create(prometheus_manager_t manager) {
    return prometheus_metric_type_create(
        manager, prometheus_metric_gauge, sizeof(struct prometheus_gauge));
}
