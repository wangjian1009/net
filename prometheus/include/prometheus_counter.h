#ifndef PROMETHEUS_COUNTER_H_INCLEDED
#define PROMETHEUS_COUNTER_H_INCLEDED
#include "prometheus_types.h"

CPE_BEGIN_DECL

prometheus_counter_t
prometheus_counter_create(
    prometheus_manager_t manager,
    const char *name, const char *help,
    size_t label_key_count, const char **label_keys);

void prometheus_counter_free(prometheus_counter_t counter);

prometheus_counter_t prometheus_counter_cast(prometheus_metric_t metric);

int prometheus_counter_inc(prometheus_counter_t counter, const char **label_values);
int prometheus_counter_add(prometheus_counter_t counter, double r_value, const char **label_values);
int prometheus_counter_set(prometheus_counter_t counter, double r_value, const char **label_values);

CPE_END_DECL

#endif
