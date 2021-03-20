#include "cpe/pal/pal_strings.h"
#include "prometheus_manager_i.h"
#include "prometheus_metric_i.h"
#include "prometheus_metric_type_i.h"
#include "prometheus_counter_i.h"
#include "prometheus_gauge_i.h"

prometheus_manager_t
prometheus_manager_create(mem_allocrator_t alloc, error_monitor_t em) {
    prometheus_manager_t manager = mem_alloc(alloc, sizeof(struct prometheus_manager));
    if (manager == NULL) {
        CPE_ERROR(em, "prometheus: manager: alloc fail!");
        return NULL;
    }

    manager->m_alloc = alloc;
    manager->m_em = em;

    bzero(manager->m_metric_types, sizeof(manager->m_metric_types));

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
    
    return manager;
}

void prometheus_manager_free(prometheus_manager_t manager) {
    uint8_t i;
    for(i = 0; i < CPE_ARRAY_SIZE(manager->m_metric_types); ++i) {
        if (manager->m_metric_types[i]) {
            prometheus_metric_type_free(manager->m_metric_types[i]);
        }
    }

    mem_buffer_clear(&manager->m_tmp_buffer);
    
    mem_free(manager->m_alloc, manager);
}
