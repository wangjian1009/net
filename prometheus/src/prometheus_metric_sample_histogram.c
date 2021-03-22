#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/stream_buffer.h"
#include "prometheus_metric_sample_histogram_i.h"
#include "prometheus_histogram_buckets_i.h"
#include "prometheus_metric_sample_i.h"

static const char *
prometheus_metric_sample_histogram_l_value_for_bucket(
    mem_buffer_t buffer, prometheus_metric_sample_histogram_t histogram, const char **label_values, double bucket);

static const char *
prometheus_metric_sample_histogram_l_value_for_inf(
    mem_buffer_t buffer, prometheus_metric_sample_histogram_t histogram, const char ** label_values);

static int prometheus_metric_sample_histogram_init_bucket_samples(
    prometheus_metric_sample_histogram_t histogram, const char **label_values);

static int prometheus_metric_sample_histogram_init_inf(
    prometheus_metric_sample_histogram_t histogram, const char **label_values);

static int prometheus_metric_sample_histogram_init_count(
    prometheus_metric_sample_histogram_t histogram, const char **label_values);

static int prometheus_metric_sample_histogram_init_summary(
    prometheus_metric_sample_histogram_t histogram, const char **label_values);

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
    histogram->m_buckets = buckets;
    
    if (cpe_hash_table_init(
            &histogram->m_samples,
            manager->m_alloc,
            (cpe_hash_fun_t) prometheus_metric_sample_hash,
            (cpe_hash_eq_t) prometheus_metric_sample_eq,
            CPE_HASH_OBJ2ENTRY(prometheus_metric_sample, m_owner_histogram.m_hh),
            -1) != 0)
    {
        mem_free(manager->m_alloc, histogram);
        return NULL;
    }
    
    histogram->m_l_value = cpe_str_mem_dup(manager->m_alloc, l_value);
    if (histogram->m_l_value == NULL) {
        CPE_ERROR(
            manager->m_em, "prometheus: %s: %s: create: dup l_value fail",
            metric->m_name, l_value);
        cpe_hash_table_fini(&histogram->m_samples);
        mem_free(manager->m_alloc, histogram);
        return NULL;
    }

    cpe_hash_entry_init(&histogram->m_hh_for_metric);
    if (cpe_hash_table_insert_unique(&metric->m_sample_histograms, histogram) != 0) {
        CPE_ERROR(
            manager->m_em, "prometheus: %s: %s: create: duplicate",
            metric->m_name, l_value);
        mem_free(manager->m_alloc, histogram->m_l_value);
        cpe_hash_table_fini(&histogram->m_samples);
        mem_free(manager->m_alloc, histogram);
        return NULL;
    }
    
    /*初始化完成，后续可以调用free */
    if (prometheus_metric_sample_histogram_init_bucket_samples(histogram, label_values) != 0) {
        prometheus_metric_sample_histogram_free(histogram);
        return NULL;
    }
    
    if (prometheus_metric_sample_histogram_init_inf(histogram, label_values) != 0) {
        prometheus_metric_sample_histogram_free(histogram);
        return NULL;
    }

    if (prometheus_metric_sample_histogram_init_count(histogram, label_values) != 0) {
        prometheus_metric_sample_histogram_free(histogram);
        return NULL;
    }

    if (prometheus_metric_sample_histogram_init_summary(histogram, label_values) != 0) {
        prometheus_metric_sample_histogram_free(histogram);
        return NULL;
    }
    
    return histogram;
}

void prometheus_metric_sample_histogram_free(prometheus_metric_sample_histogram_t histogram) {
    prometheus_metric_t metric = histogram->m_metric;
    prometheus_manager_t manager = metric->m_manager;

    while(!TAILQ_EMPTY(&histogram->m_samples_in_order)) {
        prometheus_metric_sample_free(TAILQ_FIRST(&histogram->m_samples_in_order));
    }

    assert(cpe_hash_table_count(&histogram->m_samples) == 0);
    cpe_hash_table_fini(&histogram->m_samples);

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

static int prometheus_metric_sample_histogram_init_bucket_samples(
    prometheus_metric_sample_histogram_t histogram, const char **label_values)
{
    prometheus_manager_t manager = histogram->m_metric->m_manager;
    
    int bucket_count = prometheus_histogram_buckets_count(histogram->m_buckets);

    // For each bucket, create an prometheus_metric_sample_t with an appropriate l_value and default value of 0.0. The
    // l_value will contain the metric name, user labels, and finally, the le label and bucket value.
    for (int i = 0; i < bucket_count; i++) {
        const char *l_value =
            prometheus_metric_sample_histogram_l_value_for_bucket(
                &manager->m_tmp_buffer, histogram, label_values, histogram->m_buckets->m_upper_bounds[i]);
        if (l_value == NULL) return -1;

    /*     r = prometheus_linked_list_append(histogram->l_value_list, prometheus_strdup(l_value)); */
    /*     if (r) return r; */

        char bucket_key_buf[50];
        const char * bucket_key = prometheus_metric_sample_histogram_bucket_to_str(
            bucket_key_buf, sizeof(bucket_key_buf), histogram->m_buckets->m_upper_bounds[i]);
        if (bucket_key == NULL) return -1;

    /*     r = prometheus_map_set(histogram->l_values, bucket_key, (char *)l_value); */
    /*     if (r) return r; */

    /*     prometheus_metric_sample_t *sample = prometheus_metric_sample_new(PROM_HISTOGRAM, l_value, 0.0); */
    /*     if (sample == NULL) return 1; */

    /*     r = prometheus_map_set(histogram->samples, l_value, sample); */
    /*     if (r) return r; */

    /*     prometheus_free((void *)bucket_key); */
    }

    return 0;
}

static int prometheus_metric_sample_histogram_init_inf(
    prometheus_metric_sample_histogram_t histogram, const char ** label_values)
{
    prometheus_manager_t manager = histogram->m_metric->m_manager;

    const char *inf_l_value = prometheus_metric_sample_histogram_l_value_for_inf(&manager->m_tmp_buffer, histogram, label_values);
    if (inf_l_value == NULL) return -1;

    /* r = prometheus_linked_list_append(histogram->l_value_list, prometheus_strdup(inf_l_value)); */
    /* if (r) return r; */

    /* r = prometheus_map_set(histogram->l_values, "+Inf", (char *)inf_l_value); */
    /* if (r) return r; */

    /* prometheus_metric_sample_t *inf_sample = prometheus_metric_sample_new(PROM_HISTOGRAM, (char *)inf_l_value, 0.0); */
    /* if (inf_sample == NULL) return 1; */

    /* return prometheus_map_set(histogram->samples, inf_l_value, inf_sample); */
    return -1;
}

static int prometheus_metric_sample_histogram_init_count(
    prometheus_metric_sample_histogram_t histogram, const char **label_values)
{
    /* r = prometheus_metric_formatter_load_l_value(histogram->metric_formatter, name, "count", label_count, label_keys, label_values); */
    /* if (r) return r; */

    /* const char *count_l_value = prometheus_metric_formatter_dump(histogram->metric_formatter); */
    /* if (count_l_value == NULL) return 1; */

    /* r = prometheus_linked_list_append(histogram->l_value_list, prometheus_strdup(count_l_value)); */
    /* if (r) return r; */

    /* r = prometheus_map_set(histogram->l_values, "count", (char *)count_l_value); */
    /* if (r) return r; */

    /* prometheus_metric_sample_t *count_sample = prometheus_metric_sample_new(PROM_HISTOGRAM, count_l_value, 0.0); */
    /* if (count_sample == NULL) return 1; */

    /* return prometheus_map_set(histogram->samples, count_l_value, count_sample); */
    return -1;
}

static int prometheus_metric_sample_histogram_init_summary(
    prometheus_metric_sample_histogram_t histogram, const char **label_values)
{
    /* r = prometheus_metric_formatter_load_l_value(histogram->metric_formatter, name, "sum", label_count, label_keys, label_values); */
    /* if (r) return r; */

    /* const char *sum_l_value = prometheus_metric_formatter_dump(histogram->metric_formatter); */
    /* if (sum_l_value == NULL) return 1; */

    /* r = prometheus_linked_list_append(histogram->l_value_list, prometheus_strdup(sum_l_value)); */
    /* if (r) return r; */

    /* r = prometheus_map_set(histogram->l_values, "sum", (char *)sum_l_value); */
    /* if (r) return r; */

    /* prometheus_metric_sample_t *sum_sample = prometheus_metric_sample_new(PROM_HISTOGRAM, sum_l_value, 0.0); */
    /* if (sum_sample == NULL) return 1; */

    /* return prometheus_map_set(histogram->samples, sum_l_value, sum_sample); */
    return -1;
}

int prometheus_metric_sample_histogram_observe(prometheus_metric_sample_histogram_t histogram, double value) {
    /* int r = 0; */

    /* // Update the counter for the proper bucket if found */
    /* int bucket_count = prometheus_histogram_buckets_count(histogram->m_buckets); */
    /* for (int i = (bucket_count - 1); i >= 0; i--) { */
    /*     if (value > histogram->m_buckets->m_upper_bounds[i]) { */
    /*         break; */
    /*     } */

    /*     char bucket_key_buf[50]; */
    /*     const char * bucket_key = */
    /*         prometheus_metric_sample_histogram_bucket_to_str( */
    /*             bucket_key_buf, sizeof(bucket_key_buf), histogram->m_buckets->m_upper_bounds[i]); */
    /*     if (bucket_key == NULL) return -1; */

    /*     const char * l_value = prometheus_map_get(histogram->l_values, bucket_key); */
    /*     if (l_value == NULL) { */
    /*         /\* prometheus_free((void *)bucket_key); *\/ */
    /*         /\* PROM_METRIC_SAMPLE_HISTOGRAM_OBSERVE_HANDLE_UNLOCK(1); *\/ */
    /*     } */

    /*     prometheus_metric_sample_t *sample = prometheus_map_get(histogram->samples, l_value); */
    /*     if (sample == NULL) { */
    /*         PROM_METRIC_SAMPLE_HISTOGRAM_OBSERVE_HANDLE_UNLOCK(1) */
    /*             } */

    /*     if (prometheus_metric_sample_add(sample, 1.0) != 0) return -1; */
    /* } */

    /* // Update the +Inf and count samples */
    /* const char *inf_l_value = prometheus_map_get(histogram->l_values, "+Inf"); */
    /* if (inf_l_value == NULL) { */
    /*     PROM_METRIC_SAMPLE_HISTOGRAM_OBSERVE_HANDLE_UNLOCK(1); */
    /* } */

    /* prometheus_metric_sample_t *inf_sample = prometheus_map_get(histogram->samples, inf_l_value); */
    /* if (inf_sample == NULL) { */
    /*     PROM_METRIC_SAMPLE_HISTOGRAM_OBSERVE_HANDLE_UNLOCK(1); */
    /* } */

    /* r = prometheus_metric_sample_add(inf_sample, 1.0); */
    /* if (r) { */
    /*     PROM_METRIC_SAMPLE_HISTOGRAM_OBSERVE_HANDLE_UNLOCK(1); */
    /* } */

    /* const char *count_l_value = prometheus_map_get(histogram->l_values, "count"); */
    /* if (count_l_value == NULL) { */
    /*     PROM_METRIC_SAMPLE_HISTOGRAM_OBSERVE_HANDLE_UNLOCK(1); */
    /* } */

    /* prometheus_metric_sample_t *count_sample = prometheus_map_get(histogram->samples, count_l_value); */
    /* if (count_sample == NULL) { */
    /*     PROM_METRIC_SAMPLE_HISTOGRAM_OBSERVE_HANDLE_UNLOCK(1); */
    /* } */

    /* r = prometheus_metric_sample_add(count_sample, 1.0); */
    /* if (r) { */
    /*     PROM_METRIC_SAMPLE_HISTOGRAM_OBSERVE_HANDLE_UNLOCK(1); */
    /* } */

    /* // Update the sum sample */
    /* const char *sum_l_value = prometheus_map_get(histogram->l_values, "sum"); */
    /* if (sum_l_value == NULL) { */
    /*     PROM_METRIC_SAMPLE_HISTOGRAM_OBSERVE_HANDLE_UNLOCK(1); */
    /* } */

    /* prometheus_metric_sample_t *sum_sample = prometheus_map_get(histogram->samples, sum_l_value); */
    /* if (sum_sample == NULL) { */
    /*     PROM_METRIC_SAMPLE_HISTOGRAM_OBSERVE_HANDLE_UNLOCK(1); */
    /* } */

    /* r = prometheus_metric_sample_add(sum_sample, value); */
    /* PROM_METRIC_SAMPLE_HISTOGRAM_OBSERVE_HANDLE_UNLOCK(r); */
    return 0;
}

static const char *
prometheus_metric_sample_histogram_l_value_for_bucket(
    mem_buffer_t buffer, prometheus_metric_sample_histogram_t histogram, const char **label_values, double bucket)
{
    prometheus_metric_t metric = histogram->m_metric;

    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    mem_buffer_clear_data(buffer);

    stream_printf((write_stream_t)&ws, "%s", metric->m_name);

    stream_putc((write_stream_t)&ws, '{');
    uint8_t i;
    for (int i = 0; i < metric->m_label_key_count; i++) {
        if (i > 0) stream_putc((write_stream_t)&ws, ',');
        stream_printf((write_stream_t)&ws, "%s=\"%s\"", metric->m_label_keys[i], label_values[i]);
    }

    if (i > 0) stream_putc((write_stream_t)&ws, ',');

    char bucket_key_buf[50];
    stream_printf(
        (write_stream_t)&ws, "le=\"%s\"",
        prometheus_metric_sample_histogram_bucket_to_str(bucket_key_buf, sizeof(bucket_key_buf), bucket));
    
    stream_putc((write_stream_t)&ws, '}');
    
    stream_putc((write_stream_t)&ws, 0);

    return mem_buffer_make_continuous(buffer, 0);
}

static const char *
prometheus_metric_sample_histogram_l_value_for_inf(
    mem_buffer_t buffer, prometheus_metric_sample_histogram_t histogram, const char ** label_values)
{
    prometheus_metric_t metric = histogram->m_metric;

    struct write_stream_buffer ws = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);
    mem_buffer_clear_data(buffer);

    stream_printf((write_stream_t)&ws, "%s", metric->m_name);

    stream_putc((write_stream_t)&ws, '{');
    uint8_t i;
    for (int i = 0; i < metric->m_label_key_count; i++) {
        if (i > 0) stream_putc((write_stream_t)&ws, ',');
        stream_printf((write_stream_t)&ws, "%s=\"%s\"", metric->m_label_keys[i], label_values[i]);
    }

    if (i > 0) stream_putc((write_stream_t)&ws, ',');
    stream_printf((write_stream_t)&ws, "le=\"+Inf\"");
    
    stream_putc((write_stream_t)&ws, '}');
    
    stream_putc((write_stream_t)&ws, 0);

    return mem_buffer_make_continuous(buffer, 0);
}

uint32_t prometheus_metric_sample_histogram_hash(prometheus_metric_sample_histogram_t sample, void * user_data) {
    return cpe_hash_str(sample->m_l_value, strlen(sample->m_l_value));
}

int prometheus_metric_sample_histogram_eq(prometheus_metric_sample_histogram_t l, prometheus_metric_sample_histogram_t r) {
    return strcmp(l->m_l_value, r->m_l_value) == 0;
}
