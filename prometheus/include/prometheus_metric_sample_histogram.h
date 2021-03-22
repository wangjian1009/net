#ifndef PROMETHEUS_METRIC_SAMPLE_HISOTGRAM_H
#define PROMETHEUS_METRIC_SAMPLE_HISOTGRAM_H
#include "prometheus_types.h"

int promethemus_metric_sample_histogram_observe(prometheus_metric_sample_histogram_t histogram, double value);

#endif
