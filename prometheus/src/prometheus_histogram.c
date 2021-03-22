#include "prometheus_histogram_i.h"
#include "prometheus_metric_i.h"
#include "prometheus_metric_type_i.h"
#include "prometheus_metric_sample_histogram_i.h"
#include "prometheus_histogram_buckets_i.h"

prometheus_histogram_t
prometheus_histogram_create(
    prometheus_manager_t manager,
    const char *name, const char *help,
    prometheus_histogram_buckets_t buckets,
    size_t label_key_count, const char **label_keys)
{
    prometheus_metric_t metric =
        prometheus_metric_create(
            manager, prometheus_metric_histogram,
            name, help, label_key_count, label_keys);
    if (metric == NULL) {
        CPE_ERROR(manager->m_em, "prometheus: %s: ceate fail", name);
        return NULL;
    }
    
    prometheus_histogram_t histogram = prometheus_metric_data(metric);
    if (buckets == NULL) {
        if (!manager->m_histogram_default_buckets) {
            manager->m_histogram_default_buckets =
                prometheus_histogram_buckets_create(
                    manager,
                    11,
                    0.005, 0.01, 0.025, 0.05, 0.1, 0.25, 0.5, 1.0, 2.5, 5.0, 10.0);
            if (manager->m_histogram_default_buckets == NULL) {
            }
        }
        metric->m_buckets = manager->m_histogram_default_buckets;
    }
    else {
        // Ensure the bucket values are increasing
        for (int i = 0; i < buckets->m_count; i++) {
            if (i == 0) continue;
            if (buckets->m_upper_bounds[i - 1] > buckets->m_upper_bounds[i]) {
                return NULL;
            }
        }
        metric->m_buckets = buckets;
    }

    return histogram;
}

void prometheus_histogram_free(prometheus_histogram_t histogram) {
    prometheus_metric_free(prometheus_metric_from_data(histogram));
}

int prometheus_histogram_observe(prometheus_histogram_t histogram, double value, const char **label_values) {
    prometheus_metric_t metric = prometheus_metric_from_data(histogram);
    
    prometheus_metric_sample_histogram_t h_sample = prometheus_metric_sample_histogram_from_labels(metric, label_values);
    if (h_sample == NULL) return -1;
    return prometheus_metric_sample_histogram_observe(h_sample, value);
}

prometheus_metric_type_t
prometheus_histogram_type_create(prometheus_manager_t manager) {
    return prometheus_metric_type_create(
        manager, prometheus_metric_histogram, sizeof(struct prometheus_histogram));
}
