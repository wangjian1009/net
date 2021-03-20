#ifndef PROMETHEUS_STRING_BUILDER_I_H
#define PROMETHEUS_STRING_BUILDER_I_H
#include "prometheus_manager_i.h"

struct prometheus_string_builder {
    prometheus_manager_t m_manager;
    char * str; /**< the target string  */
    size_t allocated; /**< the size allocated to the string in bytes */
    size_t len; /**< the length of str */
    size_t init_size; /**< the initialize size of space to allocate */
};

prometheus_string_builder_t prometheus_string_builder_create(prometheus_manager_t manager);
int prometheus_string_builder_free(prometheus_string_builder_t builder);
int prometheus_string_builder_add_str(prometheus_string_builder_t builder, const char *str);
int prometheus_string_builder_add_char(prometheus_string_builder_t builder, char c);
int prometheus_string_builder_clear(prometheus_string_builder_t builder);
int prometheus_string_buillder_truncate(prometheus_string_builder_t builder, size_t len);
size_t prometheus_string_builder_len(prometheus_string_builder_t builder);
char *prometheus_string_builder_dump(prometheus_string_builder_t builder);
char *prometheus_string_builder_str(prometheus_string_builder_t builder);

#endif
