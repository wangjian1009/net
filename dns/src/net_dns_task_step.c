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
            CPE_ERROR(manage->m_em, "dns: task_step alloc fail!");
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
