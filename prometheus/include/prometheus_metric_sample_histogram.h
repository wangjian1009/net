#ifndef PROMETHEUS_METRIC_SAMPLE_HISOTGRAM_H
#define PROMETHEUS_METRIC_SAMPLE_HISOTGRAM_H
#include "prometheus_types.h"

int prometheus_metric_sample_histogram_observe(prometheus_metric_sample_histogram_t histogram, double value);

prometheus_metric_sample_t
prometheus_metric_sample_histogram_find_sample_by_l_value(
    prometheus_metric_sample_histogram_t histogram, const char * l_value);

#endif
