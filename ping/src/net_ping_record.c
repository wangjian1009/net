#include <assert.h>
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/string_utils.h"
#include "net_address.h"
#include "net_ping_record_i.h"

net_ping_record_t
net_ping_record_create(net_ping_task_t task, net_ping_error_t error, const char * error_msg, uint32_t bytes, uint32_t ttl, uint32_t value) {
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
    record->m_idx = task->m_record_count;
    record->m_error = error;
    record->m_error_msg = error_msg ? cpe_str_mem_dup(mgr->m_alloc, error_msg) : NULL;
    record->m_to_notify = 1;
    record->m_bytes = bytes;
    record->m_ttl = ttl;
    record->m_value = value;

    task->m_record_count++;
    TAILQ_INSERT_TAIL(&task->m_records, record, m_next);
    net_ping_task_set_to_notify(task, 1);

    return record;
}

void net_ping_record_free(net_ping_record_t record) {
    net_ping_task_t task = record->m_task;
    net_ping_mgr_t mgr = task->m_mgr;

    if (record->m_error_msg) {
        mem_free(mgr->m_alloc, record->m_error_msg);
        record->m_error_msg = NULL;
    }
    
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

void net_ping_record_print(write_stream_t ws, net_ping_record_t record) {
    stream_printf(ws, "%d: value=%d", record->m_idx, record->m_value);
    
    if (record->m_error == net_ping_error_none) {
        switch (record->m_task->m_type) {
        case net_ping_type_icmp:
            stream_printf(ws, ", ttl=%d", record->m_ttl);
            break;
        case net_ping_type_tcp_connect:
            break;
        case net_ping_type_http:
            break;
        }
    } else {
        stream_printf(ws, "%, error=%s", net_ping_error_str(record->m_error));
        if (record->m_error_msg) {
            stream_printf(ws, "(%s)", record->m_error_msg);
        }
    }
}

const char * net_ping_record_dump(mem_buffer_t buffer, net_ping_record_t record) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

    mem_buffer_clear_data(buffer);
    net_ping_record_print((write_stream_t)&stream, record);
    stream_putc((write_stream_t)&stream, 0);

    return mem_buffer_make_continuous(buffer, 0);
}
