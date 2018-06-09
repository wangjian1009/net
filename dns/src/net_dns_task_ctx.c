#include "assert.h"
#include "net_dns_task_ctx_i.h"
#include "net_dns_entry_i.h"

net_dns_task_ctx_t
net_dns_task_ctx_create(net_dns_task_step_t step, net_dns_source_t source) {
    net_dns_manage_t manage = step->m_task->m_manage;
    net_dns_task_ctx_t ctx;

    ctx = TAILQ_FIRST(&manage->m_free_task_ctxs);
    if (ctx) {
        TAILQ_REMOVE(&manage->m_free_task_ctxs, ctx, m_next_for_step);
    }
    else {
        ctx = mem_alloc(manage->m_alloc, sizeof(struct net_dns_task_ctx) + manage->m_task_ctx_capacity);
        if (ctx == NULL) {
            CPE_ERROR(manage->m_em, "dns: task_ctx alloc fail!");
            return NULL;
        }
    }

    ctx->m_step = step;
    ctx->m_source = source;
    ctx->m_state = net_dns_task_state_init;

    TAILQ_INSERT_TAIL(&step->m_ctxs, ctx, m_next_for_step);
    TAILQ_INSERT_TAIL(&source->m_ctxs, ctx, m_next_for_source);
    
    return ctx;
}

void net_dns_task_ctx_free(net_dns_task_ctx_t ctx) {
    net_dns_task_step_t step = ctx->m_step;
    net_dns_manage_t manage = step->m_task->m_manage;

    TAILQ_REMOVE(&ctx->m_step->m_ctxs, ctx, m_next_for_step);
    TAILQ_REMOVE(&ctx->m_source->m_ctxs, ctx, m_next_for_source);

    ctx->m_step = (net_dns_task_step_t)manage;
    TAILQ_INSERT_TAIL(&manage->m_free_task_ctxs, ctx, m_next_for_step);
}

void net_dns_task_ctx_real_free(net_dns_task_ctx_t task_ctx) {
    net_dns_manage_t manage = (net_dns_manage_t)task_ctx->m_step;

    TAILQ_REMOVE(&manage->m_free_task_ctxs, task_ctx, m_next_for_step);
    mem_free(manage->m_alloc, task_ctx);
}

net_dns_task_t net_dns_task_ctx_task(net_dns_task_ctx_t ctx) {
    return ctx->m_step->m_task;
}

net_dns_task_step_t net_dns_task_ctx_step(net_dns_task_ctx_t ctx) {
    return ctx->m_step;
}

net_dns_source_t net_dns_task_ctx_source(net_dns_task_ctx_t ctx) {
    return ctx->m_source;
}

void * net_dns_task_ctx_data(net_dns_task_ctx_t ctx) {
    return ctx + 1;
}

net_dns_task_state_t net_dns_task_ctx_state(net_dns_task_ctx_t ctx) {
    return ctx->m_state;
}

void net_dns_task_ctx_set_state(net_dns_task_ctx_t ctx, net_dns_task_state_t state) {
    if (ctx->m_state == state) return;

    uint8_t is_complete =
        (ctx->m_state == net_dns_task_state_init || ctx->m_state == net_dns_task_state_runing)
        && (state == net_dns_task_state_success || state == net_dns_task_state_error);

    ctx->m_state = state;

    if (!is_complete) return;

}

void net_dns_task_ctx_start(net_dns_task_ctx_t task_ctx) {
    net_dns_task_t task = task_ctx->m_step->m_task;
    net_dns_source_t source = task_ctx->m_source;
    net_dns_manage_t manage = source->m_manage;

    if (task_ctx->m_state != net_dns_task_state_init) {
        CPE_ERROR(
            manage->m_em, "dns: %d-->%s %s already started",
            task->m_id, task->m_entry->m_hostname,
            net_dns_source_dump(net_dns_manage_tmp_buffer(manage), task_ctx->m_source));
        return;
    }

    task_ctx->m_state = net_dns_task_state_runing;

    //source->
}
