#include "prometheus_gauge_i.h"
#include "prometheus_metric_i.h"
#include "prometheus_metric_type_i.h"

prometheus_gauge_t prometheus_gauge_create(
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

prometheus_metric_type_t
prometheus_gauge_type_create(prometheus_manager_t manager) {
    return prometheus_metric_type_create(
        manager, prometheus_metric_gauge, sizeof(struct prometheus_gauge));
}
