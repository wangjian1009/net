#ifndef NET_PING_TASK_I_H_INCLEDED
#define NET_PING_TASK_I_H_INCLEDED
#include "net_ping_task.h"
#include "net_ping_mgr_i.h"

struct net_ping_task {
    net_ping_mgr_t m_mgr;
    TAILQ_ENTRY(net_ping_task) m_next;
    net_ping_task_state_t m_state;
    net_ping_record_list_t m_records;
    net_ping_processor_t m_processor;
};

void net_ping_task_real_free(net_ping_task_t task);
void net_ping_task_set_state(net_ping_task_t task, net_ping_task_state_t state);

#endif

