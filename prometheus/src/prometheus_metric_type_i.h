#ifndef PROMETHEUS_METRIC_TYPE_I_H_INCLEDED
#define PROMETHEUS_METRIC_TYPE_I_H_INCLEDED
#include "prometheus_metric_i.h"
#include "prometheus_manager_i.h"

struct prometheus_metric_type {
    prometheus_manager_t m_manager;
    prometheus_metric_category_t m_category;
    uint32_t m_capacity;
};

prometheus_metric_type_t
prometheus_metric_type_create(
    prometheus_manager_t manager, prometheus_metric_category_t category, uint32_t capacity);

void prometheus_metric_type_free(prometheus_metric_type_t type);

#endif
