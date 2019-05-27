#ifndef NET_TRANS_TASK_I_H_INCLEDED
#define NET_TRANS_TASK_I_H_INCLEDED
#include "net_trans_task.h"
#include "net_trans_manage_i.h"

NET_BEGIN_DECL

struct net_trans_task {
    net_trans_manage_t m_mgr;
    union {
        struct cpe_hash_entry m_hh_for_mgr;
        TAILQ_ENTRY(net_trans_task) m_next_for_mgr;
    };
    CURL * m_handler;
    net_watcher_t m_watcher;
    uint8_t m_debug;
    uint32_t m_id;
    net_trans_task_state_t m_state;
    net_trans_task_result_t m_result;
    net_trans_task_error_t m_error;
    struct curl_slist * m_header;
    struct mem_buffer m_buffer;
    
    /*callback*/
    uint8_t m_in_callback;
    uint8_t m_is_free;
    net_trans_task_commit_op_t m_commit_op;
    net_trans_task_write_op_t m_write_op;
    net_trans_task_progress_op_t m_progress_op;
    net_trans_task_head_op_t m_head_op;
    void * m_ctx;
    void (*m_ctx_free)(void *);
};

void net_trans_do_timeout(net_timer_t timer, void * ctx);
int net_trans_sock_cb(CURL *e, curl_socket_t s, int what, void *cbp, void *sockp);
int net_trans_timer_cb(CURLM *multi, long timeout_ms, net_trans_manage_t mgr);

/*task ops*/
int net_trans_task_set_done(net_trans_task_t task, net_trans_task_result_t result, int err);

void net_trans_task_free_all(net_trans_manage_t mgr);
void net_trans_task_real_free(net_trans_task_t task);

uint32_t net_trans_task_hash(net_trans_task_t o, void * user_data);
int net_trans_task_eq(net_trans_task_t l, net_trans_task_t r, void * user_data);

NET_END_DECL

#endif
