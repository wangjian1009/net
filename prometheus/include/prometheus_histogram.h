#ifndef PROMETHEUS_HISTOGRAM_INCLUDED
#define PROMETHEUS_HISTOGRAM_INCLUDED
#include "prometheus_types.h"

prometheus_histogram_t
prometheus_histogram_create(
    prometheus_manager_t manager,
    const char *name, const char *help,
    prometheus_histogram_buckets_t buckets,
    size_t label_key_count, const char **label_keys);

void prometheus_histogram_free(prometheus_histogram_t histogram);

int prometheus_histogram_observe(prometheus_histogram_t histogram, double value, const char **label_values);

#endif
