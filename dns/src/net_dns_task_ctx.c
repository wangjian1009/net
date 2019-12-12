#include "assert.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/utils/time_utils.h"
#include "cpe/utils/string_utils.h"
#include "net_timer.h"
#include "net_dns_task_ctx_i.h"
#include "net_dns_entry_i.h"

static void net_dns_task_ctx_do_timeout(net_timer_t timer, void * input_ctx);
static int net_dns_task_ctx_update_timeout(net_dns_task_ctx_t ctx);
static void net_dns_task_ctx_set_state_i(
    net_dns_manage_t manage, net_dns_task_t task, net_dns_task_ctx_t ctx, net_dns_task_state_t to_state);

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
            CPE_ERROR(manage->m_em, "dns-cli: task_ctx alloc fail!");
            return NULL;
        }
    }

    ctx->m_step = step;
    ctx->m_source = source;
    ctx->m_begin_time_ms = 0;
    ctx->m_complete_time_ms = 0;
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

net_dns_task_ctx_t net_dns_task_ctx_from_data(void * data) {
    return ((net_dns_task_ctx_t)data) - 1;
}

net_dns_task_state_t net_dns_task_ctx_state(net_dns_task_ctx_t ctx) {
    return ctx->m_state;
}

void net_dns_task_ctx_start(net_dns_task_ctx_t ctx) {
    net_dns_task_t task = ctx->m_step->m_task;
    net_dns_source_t source = ctx->m_source;
    net_dns_manage_t manage = source->m_manage;

    if (ctx->m_state != net_dns_task_state_init) {
        char source_name[128];
        cpe_str_dup(source_name, sizeof(source_name), net_dns_source_dump(net_dns_manage_tmp_buffer(manage), ctx->m_source));
        CPE_ERROR(
            manage->m_em, "dns-cli: query %s --> %s: already started",
            net_dns_task_dump(net_dns_manage_tmp_buffer(manage), task), source_name);
        return;
    }

    net_dns_task_ctx_set_state_i(manage, task, ctx, net_dns_task_state_runing);

    if (source->m_task_ctx_start(source, ctx) != 0) {
        char source_name[128];
        cpe_str_dup(source_name, sizeof(source_name), net_dns_source_dump(net_dns_manage_tmp_buffer(manage), ctx->m_source));
        CPE_ERROR(
            manage->m_em, "dns-cli: query %s --> %s: start fail",
            net_dns_task_dump(net_dns_manage_tmp_buffer(manage), task), source_name);
        net_dns_task_ctx_set_error(ctx);
    }
    else {
        if (ctx->m_state == net_dns_task_state_runing) {
            if (ctx->m_timeout_ms > 0u
                && (ctx->m_timeout_timer == NULL || !net_timer_is_active(ctx->m_timeout_timer)))
            {
                if (net_dns_task_ctx_update_timeout(ctx) != 0) {
                    net_dns_task_ctx_set_error(ctx);
                    return;
                }
            }

            if (manage->m_debug >= 2) {
                char source_name[128];
                cpe_str_dup(source_name, sizeof(source_name), net_dns_source_dump(net_dns_manage_tmp_buffer(manage), ctx->m_source));
                CPE_INFO(
                    manage->m_em, "dns-cli: query %s --> %s: start success",
                    net_dns_task_dump(net_dns_manage_tmp_buffer(manage), task), source_name);
            }
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

            ctx->m_timeout_timer = net_timer_auto_create(
                manage->m_schedule, net_dns_task_ctx_do_timeout, ctx);
            if (ctx->m_timeout_timer == NULL) {
                char source_name[128];
                cpe_str_dup(source_name, sizeof(source_name), net_dns_source_dump(net_dns_manage_tmp_buffer(manage), ctx->m_source));
                CPE_ERROR(
                    manage->m_em, "dns-cli: query %s --> %s: create timeout timer fail!",
                    net_dns_task_dump(net_dns_manage_tmp_buffer(manage), task),
                    source_name);
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
            char source_name[128];
            cpe_str_dup(source_name, sizeof(source_name), net_dns_source_dump(net_dns_manage_tmp_buffer(manage), ctx->m_source));
            CPE_ERROR(
                manage->m_em, "dns-cli: query %s --> %s: %.2fs timeout, restart fail",
                net_dns_task_dump(net_dns_manage_tmp_buffer(manage), task),
                source_name,
                (float)ctx->m_timeout_ms / 1000.0f);
            net_dns_task_ctx_set_error(ctx);
        }
        else {
            if (ctx->m_state == net_dns_task_state_init) {
                if (ctx->m_timeout_ms > 0u
                    && (ctx->m_timeout_timer == NULL || !net_timer_is_active(ctx->m_timeout_timer)))
                {
                    if (net_dns_task_ctx_update_timeout(ctx) != 0) {
                        net_dns_task_ctx_set_error(ctx);
                        return;
                    }
                }
            
                if (manage->m_debug >= 2) {
                    char source_name[128];
                    cpe_str_dup(source_name, sizeof(source_name), net_dns_source_dump(net_dns_manage_tmp_buffer(manage), ctx->m_source));
                    CPE_INFO(
                        manage->m_em, "dns-cli: query %s --> %s: %.2fs timeout, restart success",
                        net_dns_task_dump(net_dns_manage_tmp_buffer(manage), task),
                        source_name,
                        (float)ctx->m_timeout_ms / 1000.0f);
                }
            }
        }
    }
    else {
        char source_name[128];
        cpe_str_dup(source_name, sizeof(source_name), net_dns_source_dump(net_dns_manage_tmp_buffer(manage), ctx->m_source));
        CPE_ERROR(
            manage->m_em, "dns-cli: query %s --> %s: %.2fs timeout",
            net_dns_task_dump(net_dns_manage_tmp_buffer(manage), task), source_name,
            (float)ctx->m_timeout_ms / 1000.0f);

        net_dns_task_ctx_set_error(ctx);
    }
}

int net_dns_task_ctx_set_timeout(net_dns_task_ctx_t ctx, uint16_t timeout_ms) {
    ctx->m_timeout_ms = timeout_ms;
    return ctx->m_state == net_dns_task_state_runing ? net_dns_task_ctx_update_timeout(ctx) : 0;
}

static void net_dns_task_ctx_set_state_i(
    net_dns_manage_t manage, net_dns_task_t task, net_dns_task_ctx_t ctx, net_dns_task_state_t to_state)
{
    char source_name[128];
    cpe_str_dup(source_name, sizeof(source_name), net_dns_source_dump(net_dns_manage_tmp_buffer(manage), ctx->m_source));

    switch(to_state) {
    case net_dns_task_state_init:
        break;
    case net_dns_task_state_runing:
        ctx->m_begin_time_ms = cur_time_ms();
        if (manage->m_debug) {
            CPE_INFO(
                manage->m_em, "dns-cli: query %s --> %s: state %s ==> %s",
                net_dns_task_dump(net_dns_manage_tmp_buffer(manage), task),
                source_name,
                net_dns_task_state_str(ctx->m_state),
                net_dns_task_state_str(to_state));
        }
        break;
    case net_dns_task_state_success:
    case net_dns_task_state_empty:
    case net_dns_task_state_error:
        ctx->m_complete_time_ms = cur_time_ms();
        CPE_INFO(
            manage->m_em, "dns-cli: query %s --> %s: state %s ==> %s, %.2fs",
            net_dns_task_dump(net_dns_manage_tmp_buffer(manage), task),
            source_name,
            net_dns_task_state_str(ctx->m_state),
            net_dns_task_state_str(to_state),
            ((float)(ctx->m_complete_time_ms - ctx->m_begin_time_ms) / 1000.0f));
        break;
    }

    ctx->m_state = to_state;

    if (task->m_step_current == ctx->m_step) {
        net_dns_task_state_t step_state;
    CHECK_STEP_STATE:
        step_state = net_dns_task_step_state(task->m_step_current);
        switch(step_state) {
        case net_dns_task_state_init:
            CPE_ERROR(
                task->m_manage->m_em, "dns-cli: query %s: step start but still init",
                net_dns_task_dump(net_dns_manage_tmp_buffer(manage), task));
            net_dns_task_update_state(task, net_dns_task_state_error);
            return;
        case net_dns_task_state_runing:
            break;
        case net_dns_task_state_success:
            net_dns_task_update_state(task, net_dns_task_calc_state(task));
            break;
        case net_dns_task_state_empty:
        case net_dns_task_state_error:
            if (TAILQ_NEXT(task->m_step_current, m_next)) {
                task->m_step_current = TAILQ_NEXT(task->m_step_current, m_next);
                net_dns_task_step_start(task->m_step_current);
                goto CHECK_STEP_STATE;
            }
            else {
                net_dns_task_update_state(task, net_dns_task_calc_state(task));
            }
            break;
        }
    }
}

static void net_dns_task_ctx_set_complete_state(net_dns_task_ctx_t ctx, net_dns_task_state_t to_state) {
    net_dns_task_t task = ctx->m_step->m_task;
    net_dns_manage_t manage = task->m_manage;

    if (ctx->m_state == to_state) return;

    if (ctx->m_timeout_timer) {
        net_timer_free(ctx->m_timeout_timer);
        ctx->m_timeout_timer = NULL;
    }

    net_dns_task_ctx_set_state_i(manage, task, ctx, to_state);
}

void net_dns_task_ctx_set_success(net_dns_task_ctx_t ctx) {
    net_dns_task_ctx_set_complete_state(ctx, net_dns_task_state_success);
}

void net_dns_task_ctx_set_empty(net_dns_task_ctx_t ctx) {
    net_dns_task_ctx_set_complete_state(ctx, net_dns_task_state_empty);
}

void net_dns_task_ctx_set_error(net_dns_task_ctx_t ctx) {
    net_dns_task_ctx_set_complete_state(ctx, net_dns_task_state_error);
}

int64_t net_dns_task_ctx_begin_time_ms(net_dns_task_ctx_t ctx) {
    return ctx->m_begin_time_ms;
}

int64_t net_dns_task_ctx_complete_time_ms(net_dns_task_ctx_t ctx) {
    return ctx->m_complete_time_ms;
}
