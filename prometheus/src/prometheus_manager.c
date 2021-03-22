#include <assert.h>
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/stream_buffer.h"
#include "prometheus_manager_i.h"
#include "prometheus_collector_i.h"
#include "prometheus_metric_i.h"
#include "prometheus_metric_type_i.h"
#include "prometheus_counter_i.h"
#include "prometheus_gauge_i.h"
#include "prometheus_histogram_i.h"
#include "prometheus_histogram_buckets_i.h"

prometheus_manager_t
prometheus_manager_create(mem_allocrator_t alloc, error_monitor_t em) {
    prometheus_manager_t manager = mem_alloc(alloc, sizeof(struct prometheus_manager));
    if (manager == NULL) {
        CPE_ERROR(em, "prometheus: manager: alloc fail!");
        return NULL;
    }

    manager->m_alloc = alloc;
    manager->m_em = em;

    manager->m_metric_name_re = NULL;
    bzero(manager->m_metric_types, sizeof(manager->m_metric_types));

    manager->m_histogram_default_buckets = NULL;
    TAILQ_INIT(&manager->m_histogram_bucketses);

    manager->m_collector_default = NULL;
    TAILQ_INIT(&manager->m_collectors);

    mem_buffer_init(&manager->m_tmp_buffer, alloc);

    /*初始化成功，后续可以free*/
    if (prometheus_counter_type_create(manager) == NULL) {
        prometheus_manager_free(manager);
        return NULL;
    }

    if (prometheus_gauge_type_create(manager) == NULL) {
        prometheus_manager_free(manager);
        return NULL;
    }

    if (prometheus_histogram_type_create(manager) == NULL) {
        prometheus_manager_free(manager);
        return NULL;
    }
    
    manager->m_collector_default =
        prometheus_collector_create(manager, "default", 0, NULL, NULL, NULL);
    if (manager->m_collector_default == NULL) {
        prometheus_manager_free(manager);
        return NULL;
    }

    return manager;
}

void prometheus_manager_free(prometheus_manager_t manager) {
    while(!TAILQ_EMPTY(&manager->m_collectors)) {
        prometheus_collector_free(TAILQ_FIRST(&manager->m_collectors));
    }
    assert(manager->m_collector_default == NULL);

    uint8_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(manager->m_metric_types); ++i) {
        if (manager->m_metric_types[i]) {
            prometheus_metric_type_free(manager->m_metric_types[i]);
        }
    }

    if (manager->m_histogram_default_buckets) {
        prometheus_histogram_buckets_free(manager->m_histogram_default_buckets);
        manager->m_histogram_default_buckets = NULL;
    }

    while(!TAILQ_EMPTY(&manager->m_histogram_bucketses)) {
        prometheus_histogram_buckets_free(TAILQ_FIRST(&manager->m_histogram_bucketses));
    }

    if (manager->m_metric_name_re) {
        pcre2_code_free(manager->m_metric_name_re);
        manager->m_metric_name_re = NULL;
    }

    mem_buffer_clear(&manager->m_tmp_buffer);
    
    mem_free(manager->m_alloc, manager);
}

const char * prometheus_manager_collect_dump(mem_buffer_t buffer, prometheus_manager_t manager) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

    mem_buffer_clear_data(buffer);

    prometheus_manager_collect((write_stream_t)&stream, manager);
    stream_putc((write_stream_t)&stream, 0);
    
    return mem_buffer_make_continuous(buffer, 0);
}

uint8_t prometheus_manager_validate_metric_name(prometheus_manager_t manager, const char *metric_name) {
    if (manager->m_metric_name_re == NULL) {
        const char * str_regex = "^[a-zA-Z_:][a-zA-Z0-9_:]*$";
        int reerr;
        PCRE2_SIZE reerroffset;
        manager->m_metric_name_re = pcre2_compile_8((PCRE2_SPTR8)str_regex, PCRE2_ZERO_TERMINATED, 0, &reerr, &reerroffset, NULL);
        if (manager->m_metric_name_re == NULL) {
            CPE_ERROR(
                manager->m_em, "prometheus: metric name regex: compilation of \"%s\" failed: %d, offset %d",
                str_regex, reerr, (int)reerroffset);
            return 0;
        }
    }

    pcre2_match_data * match_data = pcre2_match_data_create_from_pattern(manager->m_metric_name_re, NULL);
    int rc = pcre2_match_8(manager->m_metric_name_re, (PCRE2_SPTR8)metric_name, PCRE2_ZERO_TERMINATED, 0, 0, match_data, NULL);
    pcre2_match_data_free(match_data);

    return rc < 0 ? 0 : 1;
}
