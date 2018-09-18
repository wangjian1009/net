#ifndef NET_TRANS_SYSTEM_H_INCLEDED
#define NET_TRANS_SYSTEM_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef enum net_trans_task_state {
    net_trans_task_init
    , net_trans_task_working
    , net_trans_task_done
} net_trans_task_state_t;

typedef enum net_trans_task_result {
    net_trans_result_unknown
    , net_trans_result_ok
    , net_trans_result_error
    , net_trans_result_timeout
    , net_trans_result_cancel
} net_trans_task_result_t;

typedef enum net_trans_method {
    net_trans_method_get,
    net_trans_method_post,
} net_trans_method_t;

typedef struct net_trans_manage * net_trans_manage_t;
typedef struct net_trans_task * net_trans_task_t;

typedef void (*net_trans_task_commit_op_t)(net_trans_task_t task, void * ctx, void * data, size_t data_size);
typedef void (*net_trans_task_progress_op_t)(net_trans_task_t task, void * ctx, double dltotal, double dlnow);
typedef void (*net_trans_task_write_op_t)(net_trans_task_t task, void * ctx, void * data, size_t data_size);

NET_END_DECL

#endif
