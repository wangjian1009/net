#include "assert.h"
#include "net_dns_task_step_i.h"
#include "net_dns_task_ctx_i.h"

net_dns_task_step_t
net_dns_task_step_create(net_dns_task_t task) {
    net_dns_manage_t manage = task->m_manage;
    net_dns_task_step_t step;

    step = TAILQ_FIRST(&manage->m_free_task_steps);
    if (step) {
        TAILQ_REMOVE(&manage->m_free_task_steps, step, m_next);
    }
    else {
        step = mem_alloc(manage->m_alloc, sizeof(struct net_dns_task_step));
        if (step == NULL) {
            CPE_ERROR(manage->m_em, "dns-cli: task_step alloc fail!");
            return NULL;
        }
    }

    step->m_task = task;
    TAILQ_INIT(&step->m_ctxs);

    TAILQ_INSERT_TAIL(&task->m_steps, step, m_next);
    
    return step;
}

void net_dns_task_step_free(net_dns_task_step_t step) {
    net_dns_task_t task = step->m_task;
    net_dns_manage_t manage = task->m_manage;

    while(!TAILQ_EMPTY(&step->m_ctxs)) {
        net_dns_task_ctx_free(TAILQ_FIRST(&step->m_ctxs));
    }

    TAILQ_REMOVE(&task->m_steps, step, m_next);

    step->m_task = (net_dns_task_t)manage;
    TAILQ_INSERT_TAIL(&manage->m_free_task_steps, step, m_next);
}

void net_dns_task_step_real_free(net_dns_task_step_t task_step) {
    net_dns_manage_t manage = (net_dns_manage_t)task_step->m_task;

    TAILQ_REMOVE(&manage->m_free_task_steps, task_step, m_next);
    mem_free(manage->m_alloc, task_step);
}

void net_dns_task_step_start(net_dns_task_step_t step) {
    net_dns_task_ctx_t ctx;

    TAILQ_FOREACH(ctx, &step->m_ctxs, m_next_for_step) {
        net_dns_task_ctx_start(ctx);
    }
}

net_dns_task_state_t net_dns_task_step_state(net_dns_task_step_t step) {
    uint8_t init_count = 0;
    uint8_t error_count = 0;
    uint8_t empty_count = 0;
    uint8_t runing_count = 0;
    net_dns_task_ctx_t ctx;
    TAILQ_FOREACH(ctx, &step->m_ctxs, m_next_for_step) {
        switch(ctx->m_state) {
        case net_dns_task_state_init:
            init_count++;
            break;
        case net_dns_task_state_runing:
            runing_count++;
            break;
        case net_dns_task_state_success:
            return net_dns_task_state_success;
        case net_dns_task_state_empty:
            empty_count++;
            break;
        case net_dns_task_state_error:
            error_count++;
            break; 
        }
    }

    if (runing_count) {
        return net_dns_task_state_runing;
    }

    if (error_count || empty_count) {
        return init_count > 0
            ? net_dns_task_state_runing
            : (error_count
               ? net_dns_task_state_error
               : net_dns_task_state_empty);
    }

    return init_count ? net_dns_task_state_init : net_dns_task_state_success;
}

static net_dns_task_ctx_t net_dns_task_ctx_next(struct net_dns_task_ctx_it * it) {
    net_dns_task_ctx_t * data = (net_dns_task_ctx_t *)(it->m_data);
    net_dns_task_ctx_t r;
    if (*data == NULL) return NULL;
    r = *data;
    *data = TAILQ_NEXT(r, m_next_for_step);
    return r;
}

void net_dns_task_step_ctxes(net_dns_task_step_t step, net_dns_task_ctx_it_t it) {
    *(net_dns_task_ctx_t *)(it->m_data) = TAILQ_FIRST(&step->m_ctxs);
    it->next = net_dns_task_ctx_next;
}
