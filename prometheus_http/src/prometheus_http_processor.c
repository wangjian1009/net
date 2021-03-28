#include "cpe/pal/pal_unistd.h"
#include "cpe/utils/string_utils.h"
#include "net_http_svr_protocol.h"
#include "net_http_svr_mount_point.h"
#include "net_http_svr_processor.h"
#include "prometheus_http_processor_i.h"
#include "prometheus_http_request_i.h"

prometheus_http_processor_t
prometheus_http_processor_create(
    net_http_svr_protocol_t http_svr,
    prometheus_manager_t manager, error_monitor_t em, mem_allocrator_t alloc)
{
    prometheus_http_processor_t processor = mem_alloc(alloc, sizeof(struct prometheus_http_processor));
    if (processor == NULL) {
        CPE_ERROR(em, "prometheus: http: alloc processor fail!");
        return NULL;
    }

    processor->m_alloc = alloc;
    processor->m_em = em;
    processor->m_manager = manager;
    processor->m_http_svr = http_svr;
    
    processor->m_http_processor =
        net_http_svr_processor_create(
            http_svr, "prometheus", processor,
            /*env*/
            NULL,
            /*request*/
            0,
            NULL,
            NULL,
            NULL,
            prometheus_http_request_on_complete);
    if (processor->m_http_processor == NULL) {
        CPE_ERROR(em, "prometheus: http: create http processor fail!");
        mem_free(alloc, processor);
        return NULL;
    }

    net_http_svr_mount_point_t mount_point =
        net_http_svr_mount_point_mount(
            net_http_svr_protocol_root(processor->m_http_svr),
            "metric", NULL, processor->m_http_processor);

    mem_buffer_init(&processor->m_collect_buffer, alloc);
    return processor;
}

void prometheus_http_processor_free(prometheus_http_processor_t processor) {
    if (processor->m_http_processor) {
        net_http_svr_processor_free(processor->m_http_processor);
        processor->m_http_processor = NULL;
    }
    
    mem_buffer_clear(&processor->m_collect_buffer);
    
    mem_free(processor->m_alloc, processor);
}
