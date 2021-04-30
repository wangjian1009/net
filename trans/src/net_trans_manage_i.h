#ifndef NET_TRANS_MANAGE_I_H_INCLEDED
#define NET_TRANS_MANAGE_I_H_INCLEDED
#include "curl/curl.h"
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/hash.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "cpe/utils/buffer.h"
#include "net_trans_manage.h"

NET_BEGIN_DECL

typedef TAILQ_HEAD(net_trans_task_list, net_trans_task) net_trans_task_list_t;
typedef TAILQ_HEAD(net_trans_task_ds_list, net_trans_task_ds) net_trans_task_ds_list_t;

struct net_trans_manage {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    uint8_t m_debug;
    net_schedule_t m_schedule;
    net_driver_t m_driver;
    char m_name[32];
    void * m_watcher_ctx;
    net_endpoint_data_watch_fun_t m_watcher_fun;
    
    net_timer_t m_timer_event;
	CURLM * m_multi_handle;
    int m_still_running;

    struct cpe_hash_table m_tasks;

    uint32_t m_max_task_id;
    net_trans_task_list_t m_free_tasks;
    net_trans_task_ds_list_t m_free_task_dses;
};

mem_buffer_t net_trans_manage_tmp_buffer(net_trans_manage_t manage);

NET_END_DECL

#endif
