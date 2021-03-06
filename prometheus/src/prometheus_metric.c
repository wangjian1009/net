#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/stream_buffer.h"
#include "prometheus_metric_i.h"
#include "prometheus_metric_type_i.h"
#include "prometheus_metric_sample_i.h"
#include "prometheus_metric_sample_histogram_i.h"
#include "prometheus_collector_metric_i.h"

prometheus_metric_t
prometheus_metric_create(
    prometheus_manager_t manager,
    prometheus_metric_category_t category,
    const char * name, const char * help,
    uint8_t label_key_count, const char **label_keys)
{
    prometheus_metric_type_t type = manager->m_metric_types[category];
    assert(name);
    assert(help);
    assert(type);

    prometheus_metric_t metric = mem_alloc(manager->m_alloc, sizeof(struct prometheus_metric));
    metric->m_manager = manager;
    metric->m_type = type;
    metric->m_name = cpe_str_mem_dup(manager->m_alloc, name);
    metric->m_help = cpe_str_mem_dup(manager->m_alloc, help);
    metric->m_buckets = NULL;
    metric->m_label_key_count = 0;
    metric->m_label_keys = NULL;
    TAILQ_INIT(&metric->m_collectors);
    
    if (cpe_hash_table_init(
            &metric->m_samples,
            manager->m_alloc,
            (cpe_hash_fun_t) prometheus_metric_sample_hash,
            (cpe_hash_eq_t) prometheus_metric_sample_eq,
            CPE_HASH_OBJ2ENTRY(prometheus_metric_sample, m_owner_metric.m_hh),
            -1) != 0)
    {
        mem_free(manager->m_alloc, metric);
        return NULL;
    }

    if (cpe_hash_table_init(
            &metric->m_sample_histograms,
            manager->m_alloc,
            (cpe_hash_fun_t) prometheus_metric_sample_histogram_hash,
            (cpe_hash_eq_t) prometheus_metric_sample_histogram_eq,
            CPE_HASH_OBJ2ENTRY(prometheus_metric_sample_histogram, m_hh_for_metric),
            -1) != 0)
    {
        cpe_hash_table_fini(&metric->m_samples);
        mem_free(manager->m_alloc, metric);
        return NULL;
    }
    
    metric->m_label_keys = mem_alloc(manager->m_alloc, sizeof(const char *) * label_key_count);
    if (metric->m_label_keys == NULL) {
        CPE_ERROR(manager->m_em, "prometheus: %s: alloc label keys fail, count=%d", name, label_key_count);
        prometheus_metric_free(metric);
        return NULL;
    }

    uint8_t i;
    for (i = 0; i < label_key_count; i++) {
        if (strcmp(label_keys[i], "le") == 0 || strcmp(label_keys[i], "quantile") == 0) {
            CPE_ERROR(manager->m_em, "prometheus: %s: invalid label name %s", name, label_keys[i]);
            prometheus_metric_free(metric);
            return NULL;
        }
        metric->m_label_keys[metric->m_label_key_count] = cpe_str_mem_dup(manager->m_alloc, label_keys[i]);
        if (metric->m_label_keys[metric->m_label_key_count] == NULL) {
            CPE_ERROR(manager->m_em, "prometheus: %s: dup key %s fail", name, label_keys[i]);
            prometheus_metric_free(metric);
            return NULL;
        }

        metric->m_label_key_count++;
    }

    return metric;
}

void prometheus_metric_free(prometheus_metric_t metric) {
    prometheus_manager_t manager = metric->m_manager;

    while(!TAILQ_EMPTY(&metric->m_collectors)) {
        prometheus_collector_metric_free(TAILQ_FIRST(&metric->m_collectors));
    }

    if (metric->m_label_keys) {
        uint8_t i;
        for (i = 0; i < metric->m_label_key_count; ++i) {
            mem_free(manager->m_alloc, (void*)metric->m_label_keys[i]);
        }
        mem_free(manager->m_alloc, metric->m_label_keys);
        metric->m_label_keys = NULL;
    }
    metric->m_label_key_count = 0;

    prometheus_metric_sample_free_all(metric);
    prometheus_metric_sample_histogram_free_all(metric);

    if (metric->m_name) {
        mem_free(manager->m_alloc, metric->m_name);
        metric->m_name = NULL;
    }

    if (metric->m_help) {
        mem_free(manager->m_alloc, metric->m_help);
        metric->m_help = NULL;
    }
    
    cpe_hash_table_fini(&metric->m_samples);
    cpe_hash_table_fini(&metric->m_sample_histograms);
    
    mem_free(manager->m_alloc, metric);
}

prometheus_metric_category_t prometheus_metric_category(prometheus_metric_t metric) {
    return metric->m_type->m_category;
}

const char * prometheus_metric_name(prometheus_metric_t metric) {
    return metric->m_name;
}

const char * prometheus_metric_help(prometheus_metric_t metric) {
    return metric->m_help;
}

prometheus_metric_sample_t
prometheus_metric_sample_from_labels(
    prometheus_metric_t metric, const char **label_values)
{
    prometheus_manager_t manager = metric->m_manager;

    const char * l_value = prometheus_metric_dump_l_value(&manager->m_tmp_buffer, metric, label_values, NULL, 0, NULL, NULL);

    struct prometheus_metric_sample key;
    key.m_l_value = (char*)l_value;

    prometheus_metric_sample_t sample = cpe_hash_table_find(&metric->m_samples, &key);
    if (sample == NULL) {
        sample = prometheus_metric_sample_create_for_metric(metric, l_value, 0.0);
        if (sample == NULL) return NULL;
    }

    return sample;
}

prometheus_metric_sample_histogram_t
prometheus_metric_sample_histogram_from_labels(prometheus_metric_t metric, const char **label_values) {
    prometheus_manager_t manager = metric->m_manager;

    const char * l_value = prometheus_metric_dump_l_value(&manager->m_tmp_buffer, metric, label_values, NULL, 0, NULL, NULL);
    
    struct prometheus_metric_sample_histogram key;
    key.m_l_value = (char*)l_value;
    
    prometheus_metric_sample_histogram_t sample = cpe_hash_table_find(&metric->m_sample_histograms, &key);
    if (sample == NULL) {
        sample = prometheus_metric_sample_histogram_create(metric, l_value, metric->m_buckets, label_values);
        if (sample == NULL) return NULL;
    }
    return sample;
}

prometheus_metric_t prometheus_metric_from_data(void * data) {
    return ((prometheus_metric_t)data) - 1;
}

void * prometheus_metric_data(prometheus_metric_t metric) {
    return metric + 1;
}

void prometheus_metric_print_l_value(
    write_stream_t ws, prometheus_metric_t metric, const char ** label_values,
    const char * suffix,
    uint8_t addition_count, const char ** addition_keys, const char ** addition_values)
{
    stream_printf(ws, "%s", metric->m_name);

    if (suffix != NULL) {
        stream_printf(ws, "_%s", suffix);
    }

    if (metric->m_label_key_count == 0 && addition_count == 0) return;

    stream_putc(ws, '{');
    uint8_t i;
    for(i = 0; i < metric->m_label_key_count; i++) {
        if (i > 0) stream_putc(ws, ',');
        stream_printf(ws, "%s=\"%s\"", metric->m_label_keys[i], label_values[i]);
    }

    uint8_t j;
    for(j = 0; j < addition_count; ++j) {
        if (i > 0 || j > 0) stream_putc(ws, ',');
        stream_printf(ws, "%s=\"%s\"", addition_keys[j], addition_values[j]);
    }
    
    stream_putc(ws, '}');
}

char * prometheus_metric_dump_l_value(
    mem_buffer_t buffer, prometheus_metric_t metric, const char ** label_values,
    const char * suffix,
    uint8_t addition_count, const char ** addition_keys, const char ** addition_values)
{
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

    mem_buffer_clear_data(buffer);
    prometheus_metric_print_l_value(
        (write_stream_t)&stream, metric, label_values,
        suffix, addition_count, addition_keys, addition_values);
    stream_putc((write_stream_t)&stream, 0);

    return mem_buffer_make_continuous(buffer, 0);
}

const char * prometheus_metric_category_str(prometheus_metric_category_t category) {
    switch(category) {
    case prometheus_metric_counter:
        return "counter";
    case prometheus_metric_gauge:
        return "gauge";
    case prometheus_metric_histogram:
        return "histogram";
    case prometheus_metric_summary:
        return "summary";
    }
}
