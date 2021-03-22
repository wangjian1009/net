#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/stream_buffer.h"
#include "prometheus_metric_sample_histogram_i.h"
#include "prometheus_histogram_buckets_i.h"
#include "prometheus_metric_sample_i.h"

prometheus_metric_sample_histogram_t
prometheus_metric_sample_histogram_create(
    prometheus_metric_t metric, const char * l_value, prometheus_histogram_buckets_t buckets, const char **label_values)
{
    prometheus_manager_t manager = metric->m_manager;

    prometheus_metric_sample_histogram_t histogram =
        mem_alloc(manager->m_alloc, sizeof(struct prometheus_metric_sample_histogram));
    if (histogram == NULL) {
        CPE_ERROR(
            manager->m_em, "prometheus: %s: %s: create: alloc fail",
            metric->m_name, l_value);
        return NULL;
    }

    histogram->m_metric = metric;
    
    histogram->m_l_value = cpe_str_mem_dup(manager->m_alloc, l_value);
    if (histogram->m_l_value == NULL) {
        CPE_ERROR(
            manager->m_em, "prometheus: %s: %s: create: dup l_value fail",
            metric->m_name, l_value);
        mem_free(manager->m_alloc, histogram);
        return NULL;
    }

    cpe_hash_entry_init(&histogram->m_hh_for_metric);
    if (cpe_hash_table_insert_unique(&metric->m_sample_histograms, histogram) != 0) {
        CPE_ERROR(
            manager->m_em, "prometheus: %s: %s: create: duplicate",
            metric->m_name, l_value);
        mem_free(manager->m_alloc, histogram->m_l_value);
        mem_free(manager->m_alloc, histogram);
        return NULL;
    }

    TAILQ_INIT(&histogram->m_samples);
    
    /*初始化完成，后续可以调用free */
    const char * addition_key = "le";

    /*buckets */
    for (int i = 0; i < buckets->m_count; i++) {
        double upper_bound = buckets->m_upper_bounds[i];
        
        char bucket_key_buf[50];
        const char * addition_value =
            prometheus_metric_sample_histogram_bucket_to_str(
                bucket_key_buf, sizeof(bucket_key_buf), upper_bound);

        const char *l_value =
            prometheus_metric_dump_l_value(
                &manager->m_tmp_buffer, metric, label_values,
                NULL, 1, &addition_key, &addition_value);
        if (l_value == NULL) goto INIT_FAILED;

        if (prometheus_metric_sample_create_for_histogram(histogram, l_value, 0.0, upper_bound) == NULL) goto INIT_FAILED;
    }

    /*inf*/
    const char * inf_addition_value = "+Inf";
    const char * inf_l_value =
        prometheus_metric_dump_l_value(
            &manager->m_tmp_buffer, histogram->m_metric, label_values,
            NULL, 1, &addition_key, &inf_addition_value);
    if (inf_l_value == NULL) goto INIT_FAILED;
    if (prometheus_metric_sample_create_for_histogram(histogram, inf_l_value, 0.0, 0.0) == NULL) goto INIT_FAILED;

    /*count*/
    const char * count_l_value =
        prometheus_metric_dump_l_value(
            &manager->m_tmp_buffer, histogram->m_metric, label_values, "count", 0, NULL, NULL);
    if (count_l_value == NULL) goto INIT_FAILED;
    if (prometheus_metric_sample_create_for_histogram(histogram, count_l_value, 0.0, 0.0) == NULL) goto INIT_FAILED;
    
    /*summary*/
    const char * sum_l_value =
        prometheus_metric_dump_l_value(
            &manager->m_tmp_buffer, histogram->m_metric, label_values, "sum", 0, NULL, NULL);
    if (sum_l_value == NULL) goto INIT_FAILED;
    if (prometheus_metric_sample_create_for_histogram(histogram, sum_l_value, 0.0, 0.0) == NULL) goto INIT_FAILED;
    
    return histogram;

INIT_FAILED:
    prometheus_metric_sample_histogram_free(histogram);
    return NULL;
}

void prometheus_metric_sample_histogram_free(prometheus_metric_sample_histogram_t histogram) {
    prometheus_metric_t metric = histogram->m_metric;
    prometheus_manager_t manager = metric->m_manager;

    while(!TAILQ_EMPTY(&histogram->m_samples)) {
        prometheus_metric_sample_free(TAILQ_FIRST(&histogram->m_samples));
    }

    mem_free(manager->m_alloc, histogram->m_l_value);
    histogram->m_l_value = NULL;

    cpe_hash_table_remove_by_ins(&metric->m_sample_histograms, histogram);
    mem_free(manager->m_alloc, histogram);
}

void prometheus_metric_sample_histogram_free_all(prometheus_metric_t metric) {
    struct cpe_hash_it sample_it;

    cpe_hash_it_init(&sample_it, &metric->m_sample_histograms);

    prometheus_metric_sample_histogram_t sample = cpe_hash_it_next(&sample_it);
    while(sample) {
        prometheus_metric_sample_histogram_t next = cpe_hash_it_next(&sample_it);
        prometheus_metric_sample_histogram_free(sample);
        sample = next;
    }
}

char * prometheus_metric_sample_histogram_bucket_to_str(char * buf, size_t buf_capacity, double bucket) {
    snprintf(buf, buf_capacity, "%g", bucket);
    if (!strchr(buf, '.')) {
        strcat(buf, ".0");
    }
    return buf;
}

int prometheus_metric_sample_histogram_observe(prometheus_metric_sample_histogram_t histogram, double value) {
    prometheus_metric_t metric = histogram->m_metric;
    prometheus_manager_t manager = metric->m_manager;

    int rv = 0;
    
    /*sum*/
    prometheus_metric_sample_t sum_sample = TAILQ_LAST(&histogram->m_samples, prometheus_metric_sample_list);
    if (sum_sample == NULL) {
        CPE_ERROR(
            manager->m_em, "prometheus: %s: %s: observe: no sum sample",
            metric->m_name, histogram->m_l_value);
        return -1;
    }

    if (prometheus_metric_sample_add(sum_sample, value) != 0) {
        CPE_ERROR(
            manager->m_em, "prometheus: %s: %s: observe: add count sample",
            metric->m_name, histogram->m_l_value);
        rv = -1;
    }

    /*count*/
    prometheus_metric_sample_t count_sample = TAILQ_PREV(sum_sample, prometheus_metric_sample_list, m_owner_histogram.m_next);
    if (count_sample == NULL) {
        CPE_ERROR(
            manager->m_em, "prometheus: %s: %s: observe: no count sample",
            metric->m_name, histogram->m_l_value);
        return -1;
    }
    if (prometheus_metric_sample_add(count_sample, 1.0) != 0) {
        CPE_ERROR(
            manager->m_em, "prometheus: %s: %s: observe: count sample inc fail",
            metric->m_name, histogram->m_l_value);
        rv = -1;
    }

    /*inf*/
    prometheus_metric_sample_t inf_sample = TAILQ_PREV(count_sample, prometheus_metric_sample_list, m_owner_histogram.m_next);
    if (inf_sample == NULL) {
        CPE_ERROR(
            manager->m_em, "prometheus: %s: %s: observe: no inf sample",
            metric->m_name, histogram->m_l_value);
        return -1;
    }
    if (prometheus_metric_sample_add(inf_sample, 1.0) != 0) {
        CPE_ERROR(
            manager->m_em, "prometheus: %s: %s: observe: inf sample inc fail",
            metric->m_name, histogram->m_l_value);
        rv = -1;
    }

    /*buckets*/
    prometheus_metric_sample_t bucket_sample;
    for(bucket_sample = TAILQ_PREV(inf_sample, prometheus_metric_sample_list, m_owner_histogram.m_next);
        bucket_sample;
        bucket_sample = TAILQ_PREV(bucket_sample, prometheus_metric_sample_list, m_owner_histogram.m_next))
    {
        if (value > bucket_sample->m_owner_histogram.m_upper_bound) {
            break;
        }

        if (prometheus_metric_sample_add(bucket_sample, 1.0) != 0) {
            CPE_ERROR(
                manager->m_em, "prometheus: %s: %s: observe: buckeet sample inc fail",
                metric->m_name, histogram->m_l_value);
            rv = -1;
        }
    }

    return 0;
}

prometheus_metric_sample_t
prometheus_metric_sample_histogram_find_sample_by_l_value(
    prometheus_metric_sample_histogram_t histogram, const char * l_value)
{
    prometheus_metric_sample_t sample;

    TAILQ_FOREACH(sample, &histogram->m_samples, m_owner_histogram.m_next) {
        if (strcmp(sample->m_l_value, l_value) == 0) return sample;
    }

    return NULL;
}

uint32_t prometheus_metric_sample_histogram_hash(prometheus_metric_sample_histogram_t sample, void * user_data) {
    return cpe_hash_str(sample->m_l_value, strlen(sample->m_l_value));
}

int prometheus_metric_sample_histogram_eq(prometheus_metric_sample_histogram_t l, prometheus_metric_sample_histogram_t r) {
    return strcmp(l->m_l_value, r->m_l_value) == 0;
}
