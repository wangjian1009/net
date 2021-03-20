#ifndef PROMETHEUS_METRIC_H_INCLEDED
#define PROMETHEUS_METRIC_H_INCLEDED
#include "prometheus_types.h"

CPE_BEGIN_DECL

enum prometheus_metric_category {
    prometheus_metric_counter,
    prometheus_metric_gauge,
    prometheus_metric_histogram,
    prometheus_metric_summary
};

void prometheus_metric_free(prometheus_metric_t metric);

prometheus_metric_category_t prometheus_metric_category(prometheus_metric_t metric);
const char * prometheus_metric_name(prometheus_metric_t metric);
const char * prometheus_metric_help(prometheus_metric_t metric);

prometheus_metric_sample_t
prometheus_metric_sample_from_labels(
    prometheus_metric_t metric, const char **label_values);

prometheus_metric_t prometheus_metric_from_data(void * data);
void * prometheus_metric_data(prometheus_metric_t metric);

const char * prometheus_metric_category_str(prometheus_metric_category_t category);

CPE_END_DECL

#endif
