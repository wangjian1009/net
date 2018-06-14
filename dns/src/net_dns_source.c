#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/string_utils.h"
#include "net_dns_source_i.h"
#include "net_dns_task_ctx_i.h"
#include "net_dns_entry_item_i.h"

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
    net_dns_task_ctx_fini_fun_t task_ctx_fini,
    net_dns_task_ctx_start_fun_t task_ctx_start,
    net_dns_task_ctx_cancel_fun_t task_ctx_cancel)
{
    net_schedule_t schedule = manage->m_schedule;

    net_dns_source_t source = mem_alloc(manage->m_alloc, sizeof(struct net_dns_source) + capacity);
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
    source->m_task_ctx_start = task_ctx_start;
    source->m_task_ctx_cancel = task_ctx_cancel;

    TAILQ_INIT(&source->m_ctxs);
    TAILQ_INIT(&source->m_items);

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
        net_dns_task_ctx_t ctx = TAILQ_FIRST(&source->m_ctxs);
        net_dns_task_ctx_free(ctx);
    }

    while(!TAILQ_EMPTY(&source->m_items)) {
        net_dns_entry_item_t item = TAILQ_FIRST(&source->m_items);
        net_dns_entry_item_free(item);
    }
    
    source->m_fini(source);
    
    TAILQ_REMOVE(&manage->m_sources, source, m_next);
    mem_free(manage->m_alloc, source);
}

void * net_dns_source_data(net_dns_source_t source) {
    return source + 1;
}

net_dns_manage_t net_dns_source_manager(net_dns_source_t source) {
    return source->m_manage;
}

net_dns_source_t net_dns_source_from_data(void * date) {
    return ((net_dns_source_t)date) - 1;
}

void net_dns_source_print(write_stream_t ws, net_dns_source_t source) {
    source->m_dump(ws, source);
}

const char * net_dns_source_dump(mem_buffer_t buffer, net_dns_source_t source) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

    mem_buffer_clear_data(buffer);
    
    net_dns_source_print((write_stream_t)&stream, source);
    stream_putc((write_stream_t)&stream, 0);
    
    return mem_buffer_make_continuous(buffer, 0);
}

static net_dns_source_t net_dns_source_next(struct net_dns_source_it * it) {
    net_dns_source_t * data = (net_dns_source_t *)(it->m_data);
    net_dns_source_t r;
    if (*data == NULL) return NULL;
    r = *data;
    *data = TAILQ_NEXT(r, m_next);
    return r;
}

void net_dns_manage_sources(net_dns_manage_t manage, net_dns_source_it_t it) {
    *(net_dns_source_t *)(it->m_data) = TAILQ_FIRST(&manage->m_sources);
    it->next = net_dns_source_next;
}
