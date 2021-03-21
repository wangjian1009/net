#ifndef PROMETHEUS_TYPES_H_INCLEDED
#define PROMETHEUS_TYPES_H_INCLEDED
#include "cpe/pal/pal_types.h"

CPE_BEGIN_DECL

typedef enum prometheus_metric_category prometheus_metric_category_t;
typedef struct prometheus_manager * prometheus_manager_t;
typedef struct prometheus_collector * prometheus_collector_t;
typedef struct prometheus_metric * prometheus_metric_t;
typedef struct prometheus_metric_sample * prometheus_metric_sample_t;
typedef struct prometheus_collector_metric * prometheus_collector_metric_t;
typedef enum prometheus_metric_collect_state prometheus_metric_collect_state_t;

typedef struct prometheus_counter * prometheus_counter_t;
typedef struct prometheus_gauge * prometheus_gauge_t;
typedef struct prometheus_histogram * prometheus_histogram_t;

CPE_END_DECL

#endif
