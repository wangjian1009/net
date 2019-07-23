#include "net_address.h"
#include "net_ping_record_i.h"

net_ping_record_t net_ping_record_create(net_ping_task_t task) {
    net_ping_mgr_t mgr = task->m_mgr;

    net_ping_record_t record = TAILQ_FIRST(&mgr->m_free_records);
    if (record) {
        TAILQ_REMOVE(&mgr->m_free_records, record, m_next);
    }
    else {
        record = mem_alloc(mgr->m_alloc, sizeof(struct net_ping_record));
        if (record == NULL) {
            CPE_ERROR(
                mgr->m_em, "ping: %s: record: alloc fail!", 
                net_address_dump(net_ping_mgr_tmp_buffer(mgr), task->m_target));
            return NULL;
        }
    }

    record->m_task = task;
    TAILQ_INSERT_TAIL(&task->m_records, record, m_next);

    return record;
}

void net_ping_record_free(net_ping_record_t record) {
    net_ping_mgr_t mgr = record->m_task->m_mgr;
    
    TAILQ_REMOVE(&record->m_task->m_records, record, m_next);

    record->m_task = (net_ping_task_t)mgr;
    TAILQ_INSERT_TAIL(&mgr->m_free_records, record, m_next);
}

void net_ping_record_real_free(net_ping_record_t record) {
    net_ping_mgr_t mgr = (net_ping_mgr_t)record->m_task;
    TAILQ_REMOVE(&mgr->m_free_records, record, m_next);
    mem_free(mgr->m_alloc, record);
}
