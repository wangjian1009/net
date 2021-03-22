#ifndef PROMETHEUS_HISTOGRAM_BUCKETS_I_H
#define PROMETHEUS_HISTOGRAM_BUCKETS_I_H
#include "prometheus_histogram_buckets.h"
#include "prometheus_manager_i.h"

struct prometheus_histogram_buckets {
    prometheus_manager_t m_manager;
    TAILQ_ENTRY(prometheus_histogram_buckets) m_next;
    int m_count;                    /* Number of buckets */
    double * m_upper_bounds;  /* The bucket values */
};

#endif
