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
    , net_trans_result_complete
    , net_trans_result_error
    , net_trans_result_cancel
} net_trans_task_result_t;

typedef enum net_trans_task_error {
    net_trans_task_error_none
    , net_trans_task_error_timeout
    , net_trans_task_error_dns_resolve_fail
    , net_trans_task_error_remote_reset
    , net_trans_task_error_net_unreachable
    , net_trans_task_error_net_down
    , net_trans_task_error_host_unreachable
    , net_trans_task_error_connect
    , net_trans_task_error_internal = -1
} net_trans_task_error_t;

typedef enum net_trans_method {
    net_trans_method_get,
    net_trans_method_post,
} net_trans_method_t;

typedef struct net_trans_manage * net_trans_manage_t;
typedef struct net_trans_task * net_trans_task_t;
typedef struct net_trans_task_cost_info * net_trans_task_cost_info_t;

typedef void (*net_trans_task_commit_op_t)(net_trans_task_t task, void * ctx, void * data, size_t data_size);
typedef void (*net_trans_task_progress_op_t)(net_trans_task_t task, void * ctx, double dltotal, double dlnow);
typedef void (*net_trans_task_write_op_t)(net_trans_task_t task, void * ctx, void * data, size_t data_size);
typedef void (*net_trans_task_head_op_t)(net_trans_task_t task, void * ctx, const char * name, const char * value);
typedef void (*net_trans_task_sock_setup_op_t)(net_trans_task_t task, void * ctx, int fd);

uint8_t net_trans_task_error_is_network_error(net_trans_task_error_t err);

NET_END_DECL

#endif
