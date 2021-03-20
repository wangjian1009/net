#ifndef PROMETHEUS_COUNTER_I_H_INCLEDED
#define PROMETHEUS_COUNTER_I_H_INCLEDED
#include "prometheus_counter.h"
#include "prometheus_manager_i.h"

struct prometheus_counter {
    int dummy;
};

prometheus_metric_type_t prometheus_counter_type_create(prometheus_manager_t manager);

#endif
