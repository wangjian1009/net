#ifndef PROMETHEUS_METRIC_SAMPLE_H_INCLEDED
#define PROMETHEUS_METRIC_SAMPLE_H_INCLEDED
#include "prometheus_types.h"

CPE_BEGIN_DECL

int prometheus_metric_sample_add(prometheus_metric_sample_t sample, double r_value);
int prometheus_metric_sample_sub(prometheus_metric_sample_t sample, double r_value);
int prometheus_metric_sample_set(prometheus_metric_sample_t sample, double r_value);

CPE_END_DECL

#endif
