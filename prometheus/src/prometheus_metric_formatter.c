#include <assert.h>
#include "cpe/pal/pal_stdio.h"
//#include "prom_collector_t.h"
#include "prometheus_metric_formatter_i.h"
#include "prometheus_metric_i.h"
#include "prometheus_string_builder_i.h"

prometheus_metric_formatter_t
prometheus_metric_formatter_create(prometheus_manager_t manager) {
    prometheus_metric_formatter_t formatter = mem_alloc(manager->m_alloc, sizeof(struct prometheus_metric_formatter));
    
    formatter->string_builder = prometheus_string_builder_create(manager);
    if (formatter->string_builder == NULL) {
        prometheus_metric_formatter_free(formatter);
        return NULL;
    }
    formatter->err_builder = prometheus_string_builder_create(manager);
    if (formatter->err_builder == NULL) {
        prometheus_metric_formatter_free(formatter);
        return NULL;
    }
    return formatter;
}

void prometheus_metric_formatter_free(prometheus_metric_formatter_t formatter) {
    assert(formatter != NULL);

    int r = 0;
    int ret = 0;

    if (formatter->string_builder) {
        prometheus_string_builder_free(formatter->string_builder);
        formatter->string_builder = NULL;
    }

    if (formatter->err_builder) {
        prometheus_string_builder_free(formatter->err_builder);
        formatter->err_builder = NULL;
    }
    
    mem_free(formatter->m_manager->m_alloc, formatter);
}

int prometheus_metric_formatter_load_help(prometheus_metric_formatter_t formatter, const char * name, const char * help) {
    assert(formatter != NULL);
    if (formatter == NULL) return 1;

    int r = 0;

    r = prometheus_string_builder_add_str(formatter->string_builder, "# HELP ");
    if (r) return r;

    r = prometheus_string_builder_add_str(formatter->string_builder, name);
    if (r) return r;

    r = prometheus_string_builder_add_char(formatter->string_builder, ' ');
    if (r) return r;

    r = prometheus_string_builder_add_str(formatter->string_builder, help);
    if (r) return r;

    return prometheus_string_builder_add_char(formatter->string_builder, '\n');
}

int prometheus_metric_formatter_load_type(
    prometheus_metric_formatter_t formatter, const char * name, prometheus_metric_category_t metric_category)
{
    assert(formatter != NULL);
    if (formatter == NULL) return 1;

    int r = 0;

    r = prometheus_string_builder_add_str(formatter->string_builder, "# TYPE ");
    if (r) return r;

    r = prometheus_string_builder_add_str(formatter->string_builder, name);
    if (r) return r;

    r = prometheus_string_builder_add_char(formatter->string_builder, ' ');
    if (r) return r;

    r = prometheus_string_builder_add_str(formatter->string_builder, prometheus_metric_category_str(metric_category));
    if (r) return r;

    return prometheus_string_builder_add_char(formatter->string_builder, '\n');
}

int prometheus_metric_formatter_load_l_value(prometheus_metric_formatter_t formatter, const char * name, const char * suffix,
    size_t label_count, const char ** label_keys, const char ** label_values) {
    assert(formatter != NULL);
    if (formatter == NULL) return 1;

    int r = 0;

    r = prometheus_string_builder_add_str(formatter->string_builder, name);
    if (r) return r;

    if (suffix != NULL) {
        r = prometheus_string_builder_add_char(formatter->string_builder, '_');
        if (r) return r;

        r = prometheus_string_builder_add_str(formatter->string_builder, suffix);
        if (r) return r;
    }

    if (label_count == 0) return 0;

    for (int i = 0; i < label_count; i++) {
        if (i == 0) {
            r = prometheus_string_builder_add_char(formatter->string_builder, '{');
            if (r) return r;
        }
        r = prometheus_string_builder_add_str(formatter->string_builder, (const char *)label_keys[i]);
        if (r) return r;

        r = prometheus_string_builder_add_char(formatter->string_builder, '=');
        if (r) return r;

        r = prometheus_string_builder_add_char(formatter->string_builder, '"');
        if (r) return r;

        r = prometheus_string_builder_add_str(formatter->string_builder, (const char *)label_values[i]);
        if (r) return r;

        r = prometheus_string_builder_add_char(formatter->string_builder, '"');
        if (r) return r;

        if (i == label_count - 1) {
            r = prometheus_string_builder_add_char(formatter->string_builder, '}');
            if (r) return r;
        } else {
            r = prometheus_string_builder_add_char(formatter->string_builder, ',');
            if (r) return r;
        }
    }
    return 0;
}

int prometheus_metric_formatter_load_sample(prometheus_metric_formatter_t formatter, prometheus_metric_sample_t sample) {
    assert(formatter != NULL);

    int r = 0;

    /* r = prometheus_string_builder_add_str(formatter->string_builder, sample->l_value); */
    /* if (r) return r; */

    /* r = prometheus_string_builder_add_char(formatter->string_builder, ' '); */
    /* if (r) return r; */

    /* char buffer[50]; */
    /* sprintf(buffer, "%.17g", sample->r_value); */
    /* r = prometheus_string_builder_add_str(formatter->string_builder, buffer); */
    /* if (r) return r; */

    /* return prometheus_string_builder_add_char(formatter->string_builder, '\n'); */
    return 0;
}

int prometheus_metric_formatter_clear(prometheus_metric_formatter_t formatter) {
    assert(formatter != NULL);
    return prometheus_string_builder_clear(formatter->string_builder);
}

char * prometheus_metric_formatter_dump(prometheus_metric_formatter_t formatter) {
    assert(formatter != NULL);

    int r = 0;
    char * data = prometheus_string_builder_dump(formatter->string_builder);
    if (data == NULL) return NULL;
    r = prometheus_string_builder_clear(formatter->string_builder);
    if (r) {
        mem_free(formatter->m_manager->m_alloc, data);
        return NULL;
    }

    return data;
}

int prometheus_metric_formatter_load_metric(prometheus_metric_formatter_t formatter, prometheus_metric_t metric) {
    assert(formatter != NULL);
    if (formatter == NULL) return 1;

    int r = 0;

    r = prometheus_metric_formatter_load_help(formatter, metric->m_name, metric->m_help);
    if (r) return r;

    r = prometheus_metric_formatter_load_type(formatter, metric->m_name, prometheus_metric_category(metric));
    if (r) return r;

    /* for (prometheus_linked_list_node_t * current_node = metric->samples->keys->head; current_node != NULL; */
    /*      current_node = current_node->next) { */
    /*     const char * key = (const char *)current_node->item; */
    /*     if (metric->type == PROM_HISTOGRAM) { */
    /*         prometheus_metric_sample_histogram_t * hist_sample = (prometheus_metric_sample_histogram_t *)prometheus_map_get(metric->samples, key); */

    /*         if (hist_sample == NULL) return 1; */

    /*         for (prometheus_linked_list_node_t * current_hist_node = hist_sample->l_value_list->head; current_hist_node != NULL; */
    /*              current_hist_node = current_hist_node->next) { */
    /*             const char * hist_key = (const char *)current_hist_node->item; */
    /*             prometheus_metric_sample_t * sample = (prometheus_metric_sample_t *)prometheus_map_get(hist_sample->samples, hist_key); */
    /*             if (sample == NULL) return 1; */
    /*             r = prometheus_metric_formatter_load_sample(formatter, sample); */
    /*             if (r) return r; */
    /*         } */
    /*     } else { */
    /*         prometheus_metric_sample_t * sample = (prometheus_metric_sample_t *)prometheus_map_get(metric->samples, key); */
    /*         if (sample == NULL) return 1; */
    /*         r = prometheus_metric_formatter_load_sample(formatter, sample); */
    /*         if (r) return r; */
    /*     } */
    /* } */
    /* return prometheus_string_builder_add_char(formatter->string_builder, '\n'); */

    return 0;
}

/* int prometheus_metric_formatter_load_metrics(prometheus_metric_formatter_t formatter, prometheus_map_t * collectors) { */
/*     assert(formatter != NULL); */
/*     int r = 0; */
/*     for (prometheus_linked_list_node_t * current_node = collectors->keys->head; current_node != NULL; */
/*          current_node = current_node->next) { */
/*         const char * collector_name = (const char *)current_node->item; */
/*         prometheus_collector_t * collector = (prometheus_collector_t *)prometheus_map_get(collectors, collector_name); */
/*         if (collector == NULL) return 1; */

/*         prometheus_map_t * metrics = collector->collect_fn(collector); */
/*         if (metrics == NULL) return 1; */

/*         for (prometheus_linked_list_node_t * current_node = metrics->keys->head; current_node != NULL; */
/*              current_node = current_node->next) { */
/*             const char * metric_name = (const char *)current_node->item; */
/*             prometheus_metric_t * metric = (prometheus_metric_t *)prometheus_map_get(metrics, metric_name); */
/*             if (metric == NULL) return 1; */
/*             r = prometheus_metric_formatter_load_metric(formatter, metric); */
/*             if (r) return r; */
/*         } */
/*     } */
/*     return r; */
/* } */
