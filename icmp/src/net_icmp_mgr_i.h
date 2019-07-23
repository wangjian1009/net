#ifndef NET_ICMP_MGR_I_H_INCLEDED
#define NET_ICMP_MGR_I_H_INCLEDED
#include "cpe/pal/pal_queue.h" 
#include "cpe/utils/error.h"
#include "cpe/utils/memory.h"
#include "net_icmp_mgr.h"

typedef struct net_icmp_ping_processor * net_icmp_ping_processor_t;

typedef TAILQ_HEAD(net_icmp_ping_task_list, net_icmp_ping_task) net_icmp_ping_task_list_t;
typedef TAILQ_HEAD(net_icmp_ping_record_list, net_icmp_ping_record) net_icmp_ping_record_list_t;
typedef TAILQ_HEAD(net_icmp_ping_processor_list, net_icmp_ping_processor) net_icmp_ping_processor_list_t;

struct net_icmp_mgr {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    net_schedule_t m_schedule;

    /*runtime*/
    net_icmp_ping_task_list_t m_ping_tasks;
    
    /*free */
    net_icmp_ping_task_list_t m_free_ping_tasks;
    net_icmp_ping_record_list_t m_free_ping_records;
    net_icmp_ping_processor_list_t m_free_ping_processors;
};

#endif

