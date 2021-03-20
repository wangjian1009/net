#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_stdio.h"
#include "prometheus_string_builder_i.h"

#define PROM_STRING_BUILDER_INIT_SIZE 32

static int prometheus_string_builder_init(prometheus_string_builder_t builder);

prometheus_string_builder_t
prometheus_string_builder_create(prometheus_manager_t manager) {
    prometheus_string_builder_t builder = mem_alloc(manager->m_alloc, sizeof(struct prometheus_string_builder));
    if (builder == NULL) {
    }

    builder->m_manager = manager;
    builder->init_size = PROM_STRING_BUILDER_INIT_SIZE;
    
    if (prometheus_string_builder_init(builder) != 0) {
        prometheus_string_builder_free(builder);
        return NULL;
    }

    return builder;
}

int prometheus_string_builder_init(prometheus_string_builder_t builder) {
    assert(builder != NULL);

    builder->str = mem_alloc(builder->m_manager->m_alloc, builder->init_size);
    builder->str[0] = 0;
    builder->allocated = builder->init_size;
    builder->len = 0;
    return 0;
}

int prometheus_string_builder_free(prometheus_string_builder_t builder) {
    assert(builder != NULL);
    if (builder == NULL) return 0;
    mem_free(builder->m_manager->m_alloc, builder->str);
    builder->str = NULL;
    mem_free(builder->m_manager->m_alloc, builder);
    builder = NULL;
    return 0;
}

static int prometheus_string_builder_ensure_space(prometheus_string_builder_t builder, size_t add_len) {
    assert(builder != NULL);
    if (builder == NULL) return 1;
    if (add_len == 0 || builder->allocated >= builder->len + add_len + 1) return 0;
    while (builder->allocated < builder->len + add_len + 1) builder->allocated <<= 1;
    //builder->str = (char *)prometheus_realloc(builder->str, builder->allocated);
    //TODO:
    assert(0);
    return 0;
}

int prometheus_string_builder_add_str(prometheus_string_builder_t builder, const char * str) {
    assert(builder != NULL);
    int r = 0;

    if (builder == NULL) return 1;
    if (str == NULL || *str == '\0') return 0;

    size_t len = strlen(str);
    r = prometheus_string_builder_ensure_space(builder, len);
    if (r) return r;

    memcpy(builder->str + builder->len, str, len);
    builder->len += len;
    builder->str[builder->len] = '\0';
    return 0;
}

int prometheus_string_builder_add_char(prometheus_string_builder_t builder, char c) {
    assert(builder != NULL);
    int r = 0;

    if (builder == NULL) return 1;
    r = prometheus_string_builder_ensure_space(builder, 1);
    if (r) return r;

    builder->str[builder->len] = c;
    builder->len++;
    builder->str[builder->len] = '\0';
    return 0;
}

int prometheus_string_builder_truncate(prometheus_string_builder_t builder, size_t len) {
    assert(builder != NULL);
    if (builder == NULL) return 1;
    if (len >= builder->len) return 0;

    builder->len = len;
    builder->str[builder->len] = '\0';
    return 0;
}

int prometheus_string_builder_clear(prometheus_string_builder_t builder) {
    assert(builder != NULL);
    mem_free(builder->m_manager->m_alloc, builder->str);
    builder->str = NULL;
    return prometheus_string_builder_init(builder);
}

size_t prometheus_string_builder_len(prometheus_string_builder_t builder) {
    assert(builder != NULL);
    return builder->len;
}

char * prometheus_string_builder_dump(prometheus_string_builder_t builder) {
    assert(builder != NULL);
    // +1 to accommodate \0
    char * out = mem_alloc(builder->m_manager->m_alloc, (builder->len + 1) * sizeof(char));
    memcpy(out, builder->str, builder->len + 1);
    return out;
}

char * prometheus_string_builder_str(prometheus_string_builder_t builder) {
    assert(builder != NULL);
    return builder->str;
}
