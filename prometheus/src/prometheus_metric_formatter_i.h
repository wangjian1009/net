#ifndef PROMETHEUS_METRIC_FORMATTER_I_H
#define PROMETHEUS_METRIC_FORMATTER_I_H
#include "prometheus_manager_i.h"

struct prometheus_metric_formatter {
    prometheus_manager_t m_manager;
    prometheus_string_builder_t string_builder;
    prometheus_string_builder_t err_builder;
};

prometheus_metric_formatter_t prometheus_metric_formatter_create(prometheus_manager_t manager);
void prometheus_metric_formatter_free(prometheus_metric_formatter_t formatter);

int prometheus_metric_formatter_load_help(
    prometheus_metric_formatter_t formatter, const char *name, const char *help);
int prometheus_metric_formatter_load_type(
    prometheus_metric_formatter_t formatter, const char *name, prometheus_metric_category_t metric_category);

int prometheus_metric_formatter_load_l_value(
    prometheus_metric_formatter_t formatter, const char *name, const char *suffix,
    size_t label_count, const char **label_keys, const char **label_values);

int prometheus_metric_formatter_load_sample(prometheus_metric_formatter_t formatter, prometheus_metric_sample_t sample);

int prometheus_metric_formatter_load_metric(prometheus_metric_formatter_t formatter, prometheus_metric_t metric);

//int prometheus_metric_formatter_load_metrics(prometheus_metric_formatter_t formatter, prometheus_map_t *collectors);

int prometheus_metric_formatter_clear(prometheus_metric_formatter_t formatter);

char * prometheus_metric_formatter_dump(prometheus_metric_formatter_t metric_formatter);

#endif
