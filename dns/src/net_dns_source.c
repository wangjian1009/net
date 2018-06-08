#include "net_dns_source_i.h"
#include "net_dns_task_ctx_i.h"

net_dns_source_t
net_dns_source_create(
    net_dns_manage_t manage,
    /*source*/
    uint32_t capacity,
    net_dns_source_init_fun_t init,
    net_dns_source_fini_fun_t fini,
    net_dns_source_dump_fun_t dump,
    /*task_ctx*/
    uint32_t task_ctx_capacity,
    net_dns_task_ctx_init_fun_t task_ctx_init,
    net_dns_task_ctx_fini_fun_t task_ctx_fini)
{
    net_schedule_t schedule = manage->m_schedule;

    net_dns_source_t source = mem_alloc(manage->m_alloc, sizeof(struct net_dns_source));
    if (source == NULL) {
        CPE_ERROR(manage->m_em, "dns: source: alloc fail!");
        return NULL;
    }

    source->m_manage = manage;

    source->m_capacity = capacity;
    source->m_init = init;
    source->m_fini = fini;
    source->m_dump = dump;

    source->m_task_ctx_capacity = task_ctx_capacity;
    source->m_task_ctx_init = task_ctx_init;
    source->m_task_ctx_fini = task_ctx_fini;

    TAILQ_INIT(&source->m_ctxs);
    
    if (source->m_init(source) != 0) {
        mem_free(manage->m_alloc, source);
        return NULL;
    }

    if (source->m_task_ctx_capacity > manage->m_task_ctx_capacity) {
        manage->m_task_ctx_capacity = source->m_task_ctx_capacity;
    }
    
    TAILQ_INSERT_TAIL(&manage->m_sources, source, m_next);
    return source;
}

void net_dns_source_free(net_dns_source_t source) {
    net_dns_manage_t manage = source->m_manage;

    while(!TAILQ_EMPTY(&source->m_ctxs)) {
        net_dns_task_ctx_free(TAILQ_FIRST(&source->m_ctxs));
    }

    source->m_fini(source);
    
    TAILQ_REMOVE(&manage->m_sources, source, m_next);
    mem_free(manage->m_alloc, source);
}

