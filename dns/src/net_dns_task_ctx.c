#include "assert.h"
#include "net_timer.h"
#include "net_dns_task_ctx_i.h"
#include "net_dns_entry_i.h"

static void net_dns_task_ctx_do_timeout(net_timer_t timer, void * input_ctx);
static int net_dns_task_ctx_update_timeout(net_dns_task_ctx_t ctx);

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
    ctx->m_timeout_timer = NULL;
    ctx->m_timeout_ms = (uint16_t)-1;
    ctx->m_retry_count = 0;

    if (source->m_task_ctx_init(source, ctx) != 0) {
        ctx->m_step = (net_dns_task_step_t)manage;
        TAILQ_INSERT_TAIL(&manage->m_free_task_ctxs, ctx, m_next_for_step);
        return NULL;
    }
    
    TAILQ_INSERT_TAIL(&step->m_ctxs, ctx, m_next_for_step);
    TAILQ_INSERT_TAIL(&source->m_ctxs, ctx, m_next_for_source);
    
    return ctx;
}

void net_dns_task_ctx_free(net_dns_task_ctx_t ctx) {
    net_dns_task_step_t step = ctx->m_step;
    net_dns_manage_t manage = step->m_task->m_manage;

    if (ctx->m_timeout_timer) {
        net_timer_free(ctx->m_timeout_timer);
        ctx->m_timeout_timer = NULL;
    }

    ctx->m_source->m_task_ctx_fini(ctx->m_source, ctx);
    
    TAILQ_REMOVE(&ctx->m_step->m_ctxs, ctx, m_next_for_step);
    TAILQ_REMOVE(&ctx->m_source->m_ctxs, ctx, m_next_for_source);

    ctx->m_step = (net_dns_task_step_t)manage;
    TAILQ_INSERT_TAIL(&manage->m_free_task_ctxs, ctx, m_next_for_step);
}

void net_dns_task_ctx_real_free(net_dns_task_ctx_t ctx) {
    net_dns_manage_t manage = (net_dns_manage_t)ctx->m_step;

    TAILQ_REMOVE(&manage->m_free_task_ctxs, ctx, m_next_for_step);
    mem_free(manage->m_alloc, ctx);
}

net_dns_manage_t net_dns_task_ctx_manage(net_dns_task_ctx_t ctx) {
    return ctx->m_source->m_manage;
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

void net_dns_task_ctx_start(net_dns_task_ctx_t ctx) {
    net_dns_task_t task = ctx->m_step->m_task;
    net_dns_source_t source = ctx->m_source;
    net_dns_manage_t manage = source->m_manage;

    if (ctx->m_state != net_dns_task_state_init) {
        CPE_ERROR(
            manage->m_em, "dns: %d-->%s %s already started",
            task->m_id, task->m_entry->m_hostname,
            net_dns_source_dump(net_dns_manage_tmp_buffer(manage), ctx->m_source));
        return;
    }

    ctx->m_state = net_dns_task_state_runing;

    if (source->m_task_ctx_start(source, ctx) != 0) {
        CPE_ERROR(
            manage->m_em, "dns: %d-->%s %s start fail",
            task->m_id, task->m_entry->m_hostname,
            net_dns_source_dump(net_dns_manage_tmp_buffer(manage), ctx->m_source));
        net_dns_task_ctx_set_error(ctx);
    }
    else {
        if (ctx->m_state == net_dns_task_state_runing) {
            if (ctx->m_timeout_ms >= 0
                && (ctx->m_timeout_timer == NULL || !net_timer_is_active(ctx->m_timeout_timer)))
            {
                if (net_dns_task_ctx_update_timeout(ctx) != 0) {
                    net_dns_task_ctx_set_error(ctx);
                    return;
                }
            }
        
            CPE_INFO(
                manage->m_em, "dns: %d-->%s %s start success",
                task->m_id, task->m_entry->m_hostname,
                net_dns_source_dump(net_dns_manage_tmp_buffer(manage), ctx->m_source));
        }
    }
}

uint16_t net_dns_task_ctx_retry_count(net_dns_task_ctx_t ctx) {
    return ctx->m_retry_count;
}

void net_dns_task_ctx_set_retry_count(net_dns_task_ctx_t ctx, uint16_t count) {
    ctx->m_retry_count = count;
}

uint16_t net_dns_task_ctx_timeout(net_dns_task_ctx_t ctx) {
    return ctx->m_timeout_ms;
}

static int net_dns_task_ctx_update_timeout(net_dns_task_ctx_t ctx) {
    if (ctx->m_timeout_ms == (uint16_t)-1) {
        if (ctx->m_timeout_timer) {
            net_timer_free(ctx->m_timeout_timer);
            ctx->m_timeout_timer = NULL;
        }
    }
    else {
        if (ctx->m_timeout_timer == NULL) {
            net_dns_task_t task = ctx->m_step->m_task;
            net_dns_manage_t manage = task->m_manage;

            ctx->m_timeout_timer = net_timer_create(
                manage->m_driver, net_dns_task_ctx_do_timeout, ctx);
            if (ctx->m_timeout_timer == NULL) {
                CPE_ERROR(
                    manage->m_em, "dns: %d-->%s: %s create timeout timer fail!",
                    task->m_id, task->m_entry->m_hostname,
                    net_dns_source_dump(net_dns_manage_tmp_buffer(manage), ctx->m_source));
                return -1;
            }
        }

        net_timer_active(ctx->m_timeout_timer, (int32_t)ctx->m_timeout_ms);
    }

    return 0;
}

static void net_dns_task_ctx_do_timeout(net_timer_t timer, void * input_ctx) {
    net_dns_task_ctx_t ctx = input_ctx;
    net_dns_task_t task = ctx->m_step->m_task;
    net_dns_manage_t manage = task->m_manage;

    ctx->m_timeout_timer = NULL;

    assert(ctx->m_state == net_dns_task_state_runing);
    
    if (ctx->m_retry_count > 0) {
        ctx->m_retry_count--;

        if (ctx->m_source->m_task_ctx_start(ctx->m_source, ctx) != 0) {
            CPE_ERROR(
                manage->m_em, "dns: %d-->%s %s %.2fs timeout, restart fail",
                task->m_id, task->m_entry->m_hostname,
                net_dns_source_dump(net_dns_manage_tmp_buffer(manage), ctx->m_source),
                (float)ctx->m_timeout_ms / 1000.0f);
            net_dns_task_ctx_set_error(ctx);
        }
        else {
            if (ctx->m_state == net_dns_task_state_init) {
                if (ctx->m_timeout_ms >= 0
                    && (ctx->m_timeout_timer == NULL || !net_timer_is_active(ctx->m_timeout_timer)))
                {
                    if (net_dns_task_ctx_update_timeout(ctx) != 0) {
                        net_dns_task_ctx_set_error(ctx);
                        return;
                    }
                }
            
                if (manage->m_debug >= 2) {
                    CPE_INFO(
                        manage->m_em, "dns: %d-->%s %s %.2fs timeout, restart success",
                        task->m_id, task->m_entry->m_hostname,
                        net_dns_source_dump(net_dns_manage_tmp_buffer(manage), ctx->m_source),
                        (float)ctx->m_timeout_ms / 1000.0f);
                }
            }
        }
    }
    else {
        CPE_ERROR(
            manage->m_em, "dns: %d-->%s %s %.2fs timeout",
            task->m_id, task->m_entry->m_hostname,
            net_dns_source_dump(net_dns_manage_tmp_buffer(manage), ctx->m_source),
            (float)ctx->m_timeout_ms / 1000.0f);

        net_dns_task_ctx_set_error(ctx);
    }
}

int net_dns_task_ctx_set_timeout(net_dns_task_ctx_t ctx, uint16_t timeout_ms) {
    ctx->m_timeout_ms = timeout_ms;
    return ctx->m_state == net_dns_task_state_runing ? net_dns_task_ctx_update_timeout(ctx) : 0;
}

void net_dns_task_ctx_set_result(net_dns_task_ctx_t ctx, net_address_t address, uint8_t is_own) {
}

void net_dns_task_ctx_set_error(net_dns_task_ctx_t ctx) {
    net_dns_task_t task = ctx->m_step->m_task;
    net_dns_manage_t manage = task->m_manage;

    if (ctx->m_state != net_dns_task_state_runing) {
        CPE_ERROR(
            manage->m_em, "dns: %d-->%s %s set error in state %d",
            task->m_id, task->m_entry->m_hostname,
            net_dns_source_dump(net_dns_manage_tmp_buffer(manage), ctx->m_source),
            ctx->m_state);
        return;
    }

    ctx->m_state = net_dns_task_state_error;
}
