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
    net_trans_method_put,
    net_trans_method_delete,
    net_trans_method_patch,
} net_trans_method_t;
#define net_trans_method_count (net_trans_method_patch + 1)

typedef enum net_trans_http_version {
    net_trans_http_version_none, /* setting this means we don't care, and that we'd
                                    like the library to choose the best possible
                                    for us! */
    net_trans_http_version_1_0, /* please use HTTP 1.0 in the request */
    net_trans_http_version_1_1, /* please use HTTP 1.1 in the request */
    net_trans_http_version_2_0, /* please use HTTP 2 in the request */
    net_trans_http_version_2tls, /* use version 2 for HTTPS, version 1.1 for HTTP */
    net_trans_http_version_2_prior_knowledge, /* please use HTTP 2 without HTTP/1.1 */
} net_trans_http_version_t;

typedef struct net_trans_manage* net_trans_manage_t;
typedef struct net_trans_task * net_trans_task_t;
typedef struct net_trans_task_cost_info * net_trans_task_cost_info_t;

typedef enum  net_trans_task_read_op_result {
    net_trans_task_read_op_success,
    net_trans_task_read_op_abort,
    net_trans_task_read_op_pause,
} net_trans_task_read_op_result_t;
typedef net_trans_task_read_op_result_t (*net_trans_task_read_op_t)(
    net_trans_task_t task, void * ctx, void * data, uint32_t * data_sz);

typedef void (*net_trans_task_commit_op_t)(net_trans_task_t task, void * ctx, void * data, size_t data_size);
typedef void (*net_trans_task_progress_op_t)(net_trans_task_t task, void * ctx, double dltotal, double dlnow);
typedef void (*net_trans_task_write_op_t)(net_trans_task_t task, void * ctx, void * data, size_t data_size);
typedef void (*net_trans_task_head_op_t)(net_trans_task_t task, void * ctx, const char * name, const char * value);
typedef void (*net_trans_task_sock_setup_op_t)(net_trans_task_t task, void * ctx, int fd);

uint8_t net_trans_task_error_is_network_error(net_trans_task_error_t err);
const char * net_trans_method_str(net_trans_method_t method);

NET_END_DECL

#endif
