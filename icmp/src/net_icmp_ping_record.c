#include "net_icmp_ping_record_i.h"

net_icmp_ping_record_t net_icmp_ping_record_create(net_icmp_ping_task_t task) {
    net_icmp_ping_record_t record = mem_alloc(task->m_alloc, sizeof(struct net_icmp_ping_record));

    record->m_task = task;
    TAILQ_INSERT_TAIL(&task->m_records, record, m_next);

    return record;
}

void net_icmp_ping_record_free(net_icmp_ping_record_t record) {
    net_icmp_ping_task_t task = record->m_task;
    
    TAILQ_REMOVE(&task->m_records, record, m_next);

    mem_free(task->m_alloc, record);
}

