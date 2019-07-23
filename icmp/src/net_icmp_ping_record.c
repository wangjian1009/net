#include "net_icmp_ping_record_i.h"

net_icmp_ping_record_t net_icmp_ping_record_create(net_icmp_ping_task_t task) {
    net_icmp_mgr_t mgr = task->m_mgr;

    net_icmp_ping_record_t record = TAILQ_FIRST(&mgr->m_free_ping_records);
    if (record) {
        TAILQ_REMOVE(&mgr->m_free_ping_records, record, m_next);
    }
    else {
        record = mem_alloc(mgr->m_alloc, sizeof(struct net_icmp_ping_record));
        if (record == NULL) {
            CPE_ERROR(mgr->m_em, "icmp: ping: record: alloc fail!");
            return NULL;
        }
    }

    record->m_task = task;
    TAILQ_INSERT_TAIL(&task->m_records, record, m_next);

    return record;
}

void net_icmp_ping_record_free(net_icmp_ping_record_t record) {
    net_icmp_mgr_t mgr = record->m_task->m_mgr;
    
    TAILQ_REMOVE(&record->m_task->m_records, record, m_next);

    record->m_task = (net_icmp_ping_task_t)mgr;
    TAILQ_INSERT_TAIL(&mgr->m_free_ping_records, record, m_next);
}

void net_icmp_ping_record_real_free(net_icmp_ping_record_t record) {
    net_icmp_mgr_t mgr = (net_icmp_mgr_t)record->m_task;
    TAILQ_REMOVE(&mgr->m_free_ping_records, record, m_next);
    mem_free(mgr->m_alloc, record);
}
