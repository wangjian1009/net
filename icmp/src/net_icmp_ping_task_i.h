#ifndef NET_ICMP_PING_TASK_I_H_INCLEDED
#define NET_ICMP_PING_TASK_I_H_INCLEDED
#include "cpe/utils/error.h"
#include "cpe/utils/memory.h"
#include "net_icmp_ping_task.h"

typedef struct net_icmp_ping_processor * net_icmp_ping_processor_t;

typedef TAILQ_HEAD(net_icmp_ping_record_list, net_icmp_ping_record) net_icmp_ping_record_list_t;

struct net_icmp_ping_task {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    net_icmp_ping_task_state_t m_state;
    net_icmp_ping_record_list_t m_records;
    net_icmp_ping_processor_t m_processor;
};

#endif

