#ifndef NET_TRANS_MANAGE_I_H_INCLEDED
#define NET_TRANS_MANAGE_I_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/hash.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "cpe/utils/buffer.h"
#include "net_http_types.h"
#include "net_trans_manage.h"

NET_BEGIN_DECL

typedef struct net_trans_host * net_trans_host_t;
typedef struct net_trans_http_protocol * net_trans_http_protocol_t;
typedef struct net_trans_http_endpoint * net_trans_http_endpoint_t;

typedef TAILQ_HEAD(net_trans_http_endpoint_list, net_trans_http_endpoint) net_trans_http_endpoint_list_t;
typedef TAILQ_HEAD(net_trans_task_list, net_trans_task) net_trans_task_list_t;
typedef TAILQ_HEAD(net_trans_host_list, net_trans_host) net_trans_host_list_t;

struct net_trans_manage {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    uint8_t m_debug;
    net_schedule_t m_schedule;
    net_driver_t m_driver;
    void * m_watcher_ctx;
    net_endpoint_data_watch_fun_t m_watcher_fun;
    
    uint16_t m_cfg_host_endpoint_limit;
    net_trans_http_protocol_t m_http_protocol;
    
    struct cpe_hash_table m_hosts;
    struct cpe_hash_table m_tasks;
    
    uint32_t m_max_task_id;

    net_trans_host_list_t m_free_hosts;
    net_trans_task_list_t m_free_tasks;
};

mem_buffer_t net_trans_manage_tmp_buffer(net_trans_manage_t manage);

NET_END_DECL

#endif
