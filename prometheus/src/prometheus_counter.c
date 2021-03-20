#include "prometheus_counter_i.h"
#include "prometheus_metric_i.h"
#include "prometheus_metric_sample_i.h"
#include "prometheus_metric_type_i.h"

prometheus_counter_t prometheus_counter_create(
    prometheus_manager_t manager,
    const char *name, const char *help,
    size_t label_key_count, const char **label_keys)
{
    prometheus_metric_t metric =
        prometheus_metric_create(
            manager, prometheus_metric_counter,
            name, help, label_key_count, label_keys);
    if (metric == NULL) return NULL;
    
    prometheus_counter_t counter = prometheus_metric_data(metric);
    
    return counter;
}

void prometheus_counter_free(prometheus_counter_t counter) {
    prometheus_metric_free(prometheus_metric_from_data(counter));
}

int prometheus_counter_inc(prometheus_counter_t counter, const char **label_values) {
    prometheus_metric_t metric = prometheus_metric_from_data(counter);
    prometheus_metric_sample_t sample = prometheus_metric_sample_from_labels(metric, label_values);
    if (sample == NULL) return -1;

    return prometheus_metric_sample_add(sample, 1.0);
}

int prometheus_counter_add(prometheus_counter_t counter, double r_value, const char **label_values) {
    prometheus_metric_t metric = prometheus_metric_from_data(counter);
    prometheus_metric_sample_t sample = prometheus_metric_sample_from_labels(metric, label_values);
    if (sample == NULL) return -1;

    return prometheus_metric_sample_add(sample, r_value);
}

prometheus_metric_type_t
prometheus_counter_type_create(prometheus_manager_t manager) {
    return prometheus_metric_type_create(
        manager, prometheus_metric_counter, sizeof(struct prometheus_counter));
}
