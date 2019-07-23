#ifndef NET_PING_MGR_I_H_INCLEDED
#define NET_PING_MGR_I_H_INCLEDED
#include "cpe/pal/pal_queue.h" 
#include "cpe/utils/error.h"
#include "cpe/utils/memory.h"
#include "net_ping_mgr.h"

typedef struct net_ping_processor * net_ping_processor_t;

typedef TAILQ_HEAD(net_ping_task_list, net_ping_task) net_ping_task_list_t;
typedef TAILQ_HEAD(net_ping_record_list, net_ping_record) net_ping_record_list_t;
typedef TAILQ_HEAD(net_ping_processor_list, net_ping_processor) net_ping_processor_list_t;

struct net_ping_mgr {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    net_driver_t m_driver;
    net_trans_manage_t m_trans_mgr;

    /*runtime*/
    uint16_t m_icmp_ping_id_max;
    net_ping_task_list_t m_tasks;
    
    /*free */
    net_ping_task_list_t m_free_tasks;
    net_ping_record_list_t m_free_records;
    net_ping_processor_list_t m_free_processors;
};

mem_buffer_t net_ping_mgr_tmp_buffer(net_ping_mgr_t mgr);

#endif

