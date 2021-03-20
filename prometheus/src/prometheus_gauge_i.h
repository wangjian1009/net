#ifndef PROMETHEUS_GAUGE_I_H_INCLEDED
#define PROMETHEUS_GAUGE_I_H_INCLEDED
#include "prometheus_gauge.h"
#include "prometheus_manager_i.h"

struct prometheus_gauge {
    int dummy;
};

prometheus_metric_type_t prometheus_gauge_type_create(prometheus_manager_t manager);

#endif
