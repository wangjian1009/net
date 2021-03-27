#ifndef PROMETHEUS_GAUGE_H_INCLEDED
#define PROMETHEUS_GAUGE_H_INCLEDED
#include "prometheus_types.h"

CPE_BEGIN_DECL

prometheus_gauge_t
prometheus_gauge_create(
    prometheus_manager_t manager,
    const char *name, const char *help,
    size_t label_key_count, const char **label_keys);

void prometheus_gauge_free(prometheus_gauge_t gauge);

prometheus_gauge_t prometheus_gauge_cast(prometheus_metric_t metric);

int prometheus_gauge_inc(prometheus_gauge_t gauge, const char **label_values);
int prometheus_gauge_dec(prometheus_gauge_t gauge, const char **label_values);
int prometheus_gauge_add(prometheus_gauge_t gauge, double r_value, const char **label_values);
int prometheus_gauge_sub(prometheus_gauge_t gauge, double r_value, const char **label_values);
int prometheus_gauge_set(prometheus_gauge_t gauge, double r_value, const char **label_values);

CPE_END_DECL

#endif
