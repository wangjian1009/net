#ifndef PROMETHEUS_HISTOGRAM_BUCKETS_H
#define PROMETHEUS_HISTOGRAM_BUCKETS_H
#include "prometheus_types.h"

extern prometheus_histogram_buckets_t prometheus_histogram_default_buckets;

prometheus_histogram_buckets_t
prometheus_histogram_buckets_create(
    prometheus_manager_t manager, size_t count, double bucket, ...);

prometheus_histogram_buckets_t
prometheus_histogram_buckets_linear(
    prometheus_manager_t manager, double start, double width, size_t count);

prometheus_histogram_buckets_t
prometheus_histogram_buckets_exponential(
    prometheus_manager_t manager, double start, double factor, size_t count);

void prometheus_histogram_buckets_free(prometheus_histogram_buckets_t histogram_buckets);

size_t prometheus_histogram_buckets_count(prometheus_histogram_buckets_t histogram_buckets);

#endif
