#include "cpe/pal/pal_unistd.h"
#include "cpe/utils/string_utils.h"
#include "prometheus_http_processor_i.h"

prometheus_http_processor_t
prometheus_http_processor_create(
    prometheus_manager_t manager, error_monitor_t em, mem_allocrator_t alloc)
{
    prometheus_http_processor_t processor = mem_alloc(alloc, sizeof(struct prometheus_http_processor));
    if (processor == NULL) {
        CPE_ERROR(em, "prometheus: process: alloc provier fail!");
        return NULL;
    }

    processor->m_alloc = alloc;
    processor->m_em = em;
    processor->m_manager = manager;

    return processor;
}

void prometheus_http_processor_free(prometheus_http_processor_t processor) {
    mem_free(processor->m_alloc, processor);
}
