#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "net_ebb_processor_i.h"
#include "net_ebb_mount_point_i.h"
#include "net_ebb_request_i.h"

net_ebb_processor_t
net_ebb_processor_create(
    net_ebb_service_t service, const char * name, void * ctx,
    /*env*/
    net_ebb_processor_env_clear_fun_t env_clear,
    /*request*/
    uint16_t request_sz,
    net_ebb_processor_request_init_fun_t request_init,
    net_ebb_processor_request_fini_fun_t request_fini,
    net_ebb_processor_request_on_head_complete_fun_t request_on_head_complete,
    net_ebb_processor_request_on_complete_fun_t request_on_complete)
{
    net_ebb_processor_t processor;

    TAILQ_FOREACH(processor, &service->m_processors, m_next) {
        assert(TAILQ_EMPTY(&processor->m_requests));
    }
    assert(TAILQ_EMPTY(&service->m_free_requests));
    
    if (net_ebb_processor_find_by_name(service, name) != NULL) {
        CPE_ERROR(service->m_em, "ebb: processor: name %s duplicate!", name);
        return NULL;
    }
    
    processor = mem_alloc(service->m_alloc, sizeof(struct net_ebb_processor));
    if (processor == NULL) {
        CPE_ERROR(service->m_em, "ebb: processor: alloc fail!");
        return NULL;
    }

    processor->m_service = service;
    cpe_str_dup(processor->m_name, sizeof(processor->m_name), name);
    processor->m_ctx = ctx;
    processor->m_env_clear = env_clear;
    processor->m_request_sz = request_sz;
    processor->m_request_init = request_init;
    processor->m_request_fini = request_fini;
    processor->m_request_on_head_complete = request_on_head_complete;
    processor->m_request_on_complete = request_on_complete;
    
    if (processor->m_request_sz > service->m_request_sz) {
        service->m_request_sz = processor->m_request_sz;
    }

    TAILQ_INIT(&processor->m_requests);
    TAILQ_INIT(&processor->m_mount_points);
    
    TAILQ_INSERT_TAIL(&service->m_processors, processor, m_next);
    
    return processor;
}

void net_ebb_processor_free(net_ebb_processor_t processor) {
    net_ebb_service_t service = processor->m_service;

    while(!TAILQ_EMPTY(&processor->m_requests)) {
        net_ebb_request_free(TAILQ_FIRST(&processor->m_requests));
    }

    while(!TAILQ_EMPTY(&processor->m_mount_points)) {
        net_ebb_mount_point_free(TAILQ_FIRST(&processor->m_mount_points));
    }
    
    TAILQ_REMOVE(&service->m_processors, processor, m_next);
    mem_free(service->m_alloc, processor);
}
    
net_ebb_processor_t net_ebb_processor_find_by_name(net_ebb_service_t service, const char * name) {
    net_ebb_processor_t processor;

    TAILQ_FOREACH(processor, &service->m_processors, m_next) {
        if (strcmp(processor->m_name, name) == 0) return processor;
    }

    return NULL;
}

void * net_ebb_processor_ctx(net_ebb_processor_t processor) {
    return processor->m_ctx;
}
