#include "net_trans_task_ds_i.h"

net_trans_task_ds_t
net_trans_task_ds_create(
    net_trans_task_t task,
    net_trans_task_read_op_t read_cb, void * read_ctx, void (*read_ctx_free)(void *))
{
    net_trans_manage_t mgr = task->m_mgr;

    net_trans_task_ds_t task_ds = TAILQ_FIRST(&mgr->m_free_task_dses);
    if (task_ds) {
        TAILQ_REMOVE(&mgr->m_free_task_dses, task_ds, m_next);
    }
    else {
        task_ds = mem_alloc(mgr->m_alloc, sizeof(struct net_trans_task_ds));
        if (task_ds == NULL) {
            CPE_ERROR(mgr->m_em, "trans: task ds: alloc fail!");
            return NULL;
        }
    }

    task_ds->m_task = task;
    task_ds->m_read_cb = read_cb;
    task_ds->m_read_ctx = read_ctx;
    task_ds->m_read_ctx_free = read_ctx_free;

    TAILQ_INSERT_TAIL(&task->m_dses, task_ds, m_next);
    return task_ds;
}

void net_trans_task_ds_free(net_trans_task_ds_t task_ds) {
    net_trans_task_t task = task_ds->m_task;
    net_trans_manage_t mgr = task->m_mgr;

    if (task_ds->m_read_ctx_free) {
        task_ds->m_read_ctx_free(task_ds->m_read_ctx);
    }

    TAILQ_REMOVE(&task->m_dses, task_ds, m_next);

    task_ds->m_task = (void*)mgr;
    TAILQ_INSERT_TAIL(&mgr->m_free_task_dses, task_ds, m_next);
}

void net_trans_task_ds_real_free(net_trans_task_ds_t task_ds) {
    net_trans_manage_t mgr = (void*)task_ds->m_task;
    TAILQ_REMOVE(&mgr->m_free_task_dses, task_ds, m_next);
    mem_free(mgr->m_alloc, task_ds);
}

