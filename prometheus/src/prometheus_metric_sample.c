#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "prometheus_metric_sample_i.h"

prometheus_metric_sample_t
prometheus_metric_sample_create(prometheus_metric_t metric, const char * l_value, double r_value) {
    prometheus_manager_t manager = metric->m_manager;

    prometheus_metric_sample_t sample = mem_alloc(manager->m_alloc, sizeof(struct prometheus_metric_sample));
    if (sample == NULL) {
        CPE_ERROR(
            manager->m_em, "prometheus: %s: %s: create: alloc fail",
            metric->m_name, l_value);
        return NULL;
    }
    
    sample->m_metric = metric;
    sample->m_l_value = cpe_str_mem_dup(manager->m_alloc, l_value);
    if (sample->m_l_value == NULL) {
        CPE_ERROR(
            manager->m_em, "prometheus: %s: %s: create: dup l_value fail",
            metric->m_name, l_value);
        mem_free(manager->m_alloc, sample);
        return NULL;
    }

    cpe_hash_entry_init(&sample->m_hh_for_metric);
    if (cpe_hash_table_insert_unique(&metric->m_samples, sample) != 0) {
        CPE_ERROR(
            manager->m_em, "prometheus: %s: %s: create: duplicate",
            metric->m_name, l_value);
        mem_free(manager->m_alloc, sample->m_l_value);
        mem_free(manager->m_alloc, sample);
        return NULL;
    }

    sample->m_r_value = r_value;
    return sample;
}

void prometheus_metric_sample_free(prometheus_metric_sample_t sample) {
    prometheus_metric_t metric = sample->m_metric;
    prometheus_manager_t manager = metric->m_manager;

    mem_free(manager->m_alloc, sample->m_l_value);
    sample->m_l_value = NULL;

    cpe_hash_table_remove_by_ins(&metric->m_samples, sample);
    mem_free(manager->m_alloc, sample);
}

void prometheus_metric_sample_free_all(prometheus_metric_t metric) {
    struct cpe_hash_it sample_it;
    prometheus_metric_sample_t sample;

    cpe_hash_it_init(&sample_it, &metric->m_samples);

    sample = cpe_hash_it_next(&sample_it);
    while(sample) {
        prometheus_metric_sample_t next = cpe_hash_it_next(&sample_it);
        prometheus_metric_sample_free(sample);
        sample = next;
    }
}

const char * prometheus_metric_sample_l_value(prometheus_metric_sample_t sample) {
    return sample->m_l_value;
}

double prometheus_metric_sample_r_value(prometheus_metric_sample_t sample) {
    return sample->m_r_value;
}
        
int prometheus_metric_sample_add(prometheus_metric_sample_t sample, double r_value) {
    prometheus_metric_t metric = sample->m_metric;
    prometheus_manager_t manager = metric->m_manager;
    
    if (r_value < 0) {
        CPE_ERROR(
            manager->m_em, "prometheus: %s: %s: add: can`t add negative value %f",
            metric->m_name, sample->m_l_value, r_value);
        return -1;
    }

    sample->m_r_value += r_value;
    return 0;
}

int prometheus_metric_sample_sub(prometheus_metric_sample_t sample, double r_value) {
    prometheus_metric_t metric = sample->m_metric;
    prometheus_manager_t manager = metric->m_manager;
    
    if (prometheus_metric_category(metric) != prometheus_metric_gauge) {
        CPE_ERROR(
            manager->m_em,
            "prometheus: %s: %s: can`t sub for category %s",
            metric->m_name, sample->m_l_value, prometheus_metric_category_str(prometheus_metric_category(metric)));
        return -1;
    }

    sample->m_r_value -= r_value;
    return 0;
}

int prometheus_metric_sample_set(prometheus_metric_sample_t sample, double r_value) {
    prometheus_metric_t metric = sample->m_metric;
    prometheus_manager_t manager = metric->m_manager;
    
    if (prometheus_metric_category(metric) != prometheus_metric_gauge) {
        CPE_ERROR(
            metric->m_manager->m_em,
            "prometheus: %s: %s: can`t sub for category %s",
            metric->m_name, sample->m_l_value, prometheus_metric_category_str(prometheus_metric_category(metric)));
        return -1;
    }

    sample->m_r_value = r_value;
    return 0;
}
 
uint32_t prometheus_metric_sample_hash(prometheus_metric_sample_t sample, void * user_data) {
    return cpe_hash_str(sample->m_l_value, strlen(sample->m_l_value));
}

int prometheus_metric_sample_eq(prometheus_metric_sample_t l, prometheus_metric_sample_t r) {
    return strcmp(l->m_l_value, r->m_l_value) == 0;
}
