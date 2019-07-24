#include <assert.h>
#include "net_address.h"
#include "net_ping_record_i.h"

net_ping_record_t
net_ping_record_create(net_ping_task_t task, net_ping_error_t error, uint32_t bytes, uint32_t ttl, uint32_t value) {
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
                net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task));
            return NULL;
        }
    }

    record->m_task = task;
    record->m_error = error;
    record->m_bytes = bytes;
    record->m_ttl = ttl;
    record->m_value = value;

    task->m_record_count++;
    TAILQ_INSERT_TAIL(&task->m_records, record, m_next);

    return record;
}

void net_ping_record_free(net_ping_record_t record) {
    net_ping_task_t task = record->m_task;
    net_ping_mgr_t mgr = task->m_mgr;
    
    assert(task->m_record_count > 0);
    task->m_record_count--;
    TAILQ_REMOVE(&record->m_task->m_records, record, m_next);

    record->m_task = (net_ping_task_t)mgr;
    TAILQ_INSERT_TAIL(&mgr->m_free_records, record, m_next);
}

void net_ping_record_real_free(net_ping_record_t record) {
    net_ping_mgr_t mgr = (net_ping_mgr_t)record->m_task;
    TAILQ_REMOVE(&mgr->m_free_records, record, m_next);
    mem_free(mgr->m_alloc, record);
}

net_ping_error_t net_ping_record_error(net_ping_record_t record) {
    return record->m_error;
}

uint32_t net_ping_record_value(net_ping_record_t record) {
    return record->m_value;
}
