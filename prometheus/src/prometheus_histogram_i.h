#ifndef PROMETHEUS_HISTOGRAM_I_INCLUDED
#define PROMETHEUS_HISTOGRAM_I_INCLUDED
#include "prometheus_histogram.h"
#include "prometheus_manager_i.h"

struct prometheus_histogram {
    int dummy;
};

prometheus_metric_type_t prometheus_histogram_type_create(prometheus_manager_t manager);

#endif
