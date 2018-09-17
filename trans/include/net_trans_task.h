#ifndef NET_TRANS_TASK_H_INCLEDED
#define NET_TRANS_TASK_H_INCLEDED
#include "net_trans_system.h"

NET_BEGIN_DECL

net_trans_task_t net_trans_task_create(net_trans_manage_t mgr, const char *method, const char * url);
void net_trans_task_free(net_trans_task_t task);

uint32_t net_trans_task_id(net_trans_task_t task);
net_trans_task_t net_trans_task_find(net_trans_manage_t mgr, uint32_t task_id);
net_trans_manage_t net_trans_task_manage(net_trans_task_t task);

void net_trans_task_set_debug(net_trans_task_t task, uint8_t is_debug);

void net_trans_task_set_callback(
    net_trans_task_t task,
    net_trans_task_commit_op_t commit,
    net_trans_task_progress_op_t progress,
    net_trans_task_write_op_t write,
    void * ctx, void (*ctx_free)(void *));

int net_trans_task_set_timeout(net_trans_task_t task, uint64_t timeout_ms);

int net_trans_task_append_header(net_trans_task_t task, const char * header_one);

const char * net_trans_task_state_str(net_trans_task_state_t state);
const char * net_trans_task_result_str(net_trans_task_result_t result);

NET_END_DECL

#endif
