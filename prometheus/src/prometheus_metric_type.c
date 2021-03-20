#include <assert.h>
#include "prometheus_metric_type_i.h"

prometheus_metric_type_t
prometheus_metric_type_create(
    prometheus_manager_t manager, prometheus_metric_category_t category, uint32_t capacity)
{
    prometheus_metric_type_t type = mem_alloc(manager->m_alloc, sizeof(struct prometheus_metric_type));
    if (type == NULL) {
        CPE_ERROR(
            manager->m_em, "prometheus: %s: create type: alloc fail!", prometheus_metric_category_str(category));
        return NULL;
    }

    type->m_manager = manager;
    type->m_category = category;
    type->m_capacity = capacity;

    assert(manager->m_metric_types[category] == NULL);
    manager->m_metric_types[category] = type;
    return type;
}

void prometheus_metric_type_free(prometheus_metric_type_t type) {
    prometheus_manager_t manager = type->m_manager;
    
    assert(manager->m_metric_types[type->m_category] == type);
    manager->m_metric_types[type->m_category] = NULL;

    mem_free(manager->m_alloc, type);
}
