#include "cpe/pal/pal_stdarg.h"
#include "cpe/pal/pal_stdlib.h"
#include "prometheus_histogram_buckets_i.h"

prometheus_histogram_buckets_t
prometheus_histogram_buckets_create(prometheus_manager_t manager, size_t count, double bucket, ...) {
    prometheus_histogram_buckets_t buckets = mem_alloc(manager->m_alloc, sizeof(struct prometheus_histogram_buckets));
    if (buckets == NULL) {
        CPE_ERROR(manager->m_em, "prometheus: buckets create: alloc fail, count=%d", (int)count);
        return NULL;
    }
    
    double * upper_bounds = (double *)mem_alloc(manager->m_alloc, sizeof(double) * count);
    if (upper_bounds == NULL) {
        CPE_ERROR(manager->m_em, "prometheus: buckets create: alloc upper bounds fail, count=%d", (int)count);
        mem_free(manager->m_alloc, buckets);
        return NULL;
    }

    upper_bounds[0] = bucket;
    if (count > 0) {
        va_list arg_list;
        va_start(arg_list, bucket);
        for (int i = 1; i < count; i++) {
            upper_bounds[i] = va_arg(arg_list, double);
        }
        va_end(arg_list);
    }

    buckets->m_manager = manager;
    buckets->m_count = count;
    buckets->m_upper_bounds = upper_bounds;
    
    TAILQ_INSERT_TAIL(&manager->m_histogram_bucketses, buckets, m_next);
    return buckets;
}

prometheus_histogram_buckets_t
prometheus_histogram_buckets_linear(prometheus_manager_t manager, double start, double width, size_t count) {
    if (count <= 1) {
        CPE_ERROR(manager->m_em, "prometheus: buckets linear: counet error, count=%d", (int)count);
        return NULL;
    }

    prometheus_histogram_buckets_t buckets = mem_alloc(manager->m_alloc, sizeof(struct prometheus_histogram_buckets));
    if (buckets == NULL) {
        CPE_ERROR(manager->m_em, "prometheus: buckets linear: alloc fail, cont=%d", (int)count);
        return NULL;
    }

    double *upper_bounds = (double *)mem_alloc(manager->m_alloc, sizeof(double) * count);
    if (upper_bounds == NULL) {
        CPE_ERROR(manager->m_em, "prometheus: buckets linear: alloc upper bounds fail, count=%d", (int)count);
        mem_free(manager->m_alloc, buckets);
        return NULL;
    }

    upper_bounds[0] = start;
    for (size_t i = 1; i < count; i++) {
        upper_bounds[i] = upper_bounds[i - 1] + width;
    }

    buckets->m_manager = manager;
    buckets->m_upper_bounds = upper_bounds;
    buckets->m_count = count;

    TAILQ_INSERT_TAIL(&manager->m_histogram_bucketses, buckets, m_next);
    return buckets;
}

prometheus_histogram_buckets_t
prometheus_histogram_buckets_exponential(prometheus_manager_t manager, double start, double factor, size_t count) {
    if (count < 1) {
        CPE_ERROR(manager->m_em, "prometheus: buckets exponential: count must be less than 1");
        return NULL;
    }

    if (start <= 0) {
        CPE_ERROR(manager->m_em, "prometheus: buckets exponential: start must be less than or equal to 0");
        return NULL;
    }

    if (factor <= 1) {
        CPE_ERROR(manager->m_em, "prometheus: buckets exponential: factor must be less than or equal to 1");
        return NULL;
    }

    prometheus_histogram_buckets_t buckets = mem_alloc(manager->m_alloc, sizeof(struct prometheus_histogram_buckets));
    if (buckets == NULL) {
        CPE_ERROR(manager->m_em, "prometheus: buckets exponential: alloc fail, count=%d", (int)count);
        return NULL;
    }

    double * upper_bounds = mem_alloc(manager->m_alloc, sizeof(double) * count);
    if (upper_bounds == NULL) {
        CPE_ERROR(manager->m_em, "prometheus: buckets exponential: alloc upper bounds fail, count=%d", (int)count);
        mem_free(manager->m_alloc, buckets);
        return NULL;
    }
    
    upper_bounds[0] = start;
    for (size_t i = 1; i < count; i++) {
        upper_bounds[i] = upper_bounds[i - 1] * factor;
    }

    buckets->m_manager = manager;
    buckets->m_upper_bounds = upper_bounds;
    buckets->m_count = count;

    TAILQ_INSERT_TAIL(&manager->m_histogram_bucketses, buckets, m_next);
    return buckets;
}

void prometheus_histogram_buckets_free(prometheus_histogram_buckets_t buckets) {
    prometheus_manager_t manager = buckets->m_manager;

    if (buckets->m_upper_bounds) {
        mem_free(manager->m_alloc, buckets->m_upper_bounds);
        buckets->m_upper_bounds = NULL;
    }

    TAILQ_REMOVE(&manager->m_histogram_bucketses, buckets, m_next);

    mem_free(manager->m_alloc, buckets);
}

size_t prometheus_histogram_buckets_count(prometheus_histogram_buckets_t buckets) {
    return buckets->m_count;
}
