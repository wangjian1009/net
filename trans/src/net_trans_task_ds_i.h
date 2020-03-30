#ifndef NET_TRANS_TASK_DS_I_H_INCLEDED
#define NET_TRANS_TASK_DS_I_H_INCLEDED
#include "net_trans_task_i.h"

NET_BEGIN_DECL

struct net_trans_task_ds {
    net_trans_task_t m_task;
    TAILQ_ENTRY(net_trans_task_ds) m_next;

    /*readd cb*/
    net_trans_task_read_op_t m_read_cb;
    void * m_read_ctx;
    void (*m_read_ctx_free)(void *);
};

net_trans_task_ds_t
net_trans_task_ds_create(
    net_trans_task_t task,
    net_trans_task_read_op_t read_cb, void * read_ctx, void (*read_ctx_free)(void *));

void net_trans_task_ds_free(net_trans_task_ds_t task_ds);

void net_trans_task_ds_real_free(net_trans_task_ds_t task_ds);

NET_END_DECL

#endif
