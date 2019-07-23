#include <assert.h>
#include "cpe/utils/stream_buffer.h"
#include "cpe/utils/string_utils.h"
#include "net_schedule.h"
#include "net_address.h"
#include "net_ping_task_i.h"
#include "net_ping_record_i.h"
#include "net_ping_processor_i.h"

static net_ping_task_t net_ping_task_create_i(net_ping_mgr_t mgr, net_address_t target) {
    net_ping_task_t task = TAILQ_FIRST(&mgr->m_free_tasks);
    if (task) {
        TAILQ_REMOVE(&mgr->m_free_tasks, task, m_next);
    }
    else {
        task = mem_alloc(mgr->m_alloc, sizeof(struct net_ping_task));
        if (task == NULL) {
            CPE_ERROR(mgr->m_em, "ping: %s: alloc fail!", net_address_dump(net_ping_mgr_tmp_buffer(mgr), target));
            return NULL;
        }
    }

    task->m_mgr = mgr;
    task->m_state = net_ping_task_state_init;
    task->m_processor = NULL;
    TAILQ_INIT(&task->m_records);

    task->m_target = net_address_copy(mgr->m_schedule, target);
    if (task->m_target == NULL) {
        CPE_ERROR(mgr->m_em, "ping: %s: dup target address fail!", net_address_dump(net_ping_mgr_tmp_buffer(mgr), target));
        TAILQ_INSERT_TAIL(&mgr->m_free_tasks, task, m_next);
        return NULL;
    }
    
    return task;
}

net_ping_task_t net_ping_task_create_icmp(net_ping_mgr_t mgr, net_address_t target) {
    net_ping_task_t task = net_ping_task_create_i(mgr, target);
    task->m_type = net_ping_type_icmp;
    return task;
}

net_ping_task_t net_ping_task_create_tcp_connect(net_ping_mgr_t mgr, net_address_t target) {
    net_ping_task_t task = net_ping_task_create_i(mgr, target);
    task->m_type = net_ping_type_tcp_connect;
    return task;
}

net_ping_task_t net_ping_task_create_http(net_ping_mgr_t mgr, net_address_t target, const char * path) {
    net_ping_task_t task = net_ping_task_create_i(mgr, target);
    task->m_type = net_ping_type_http;

    task->m_http.m_path = cpe_str_mem_dup(mgr->m_alloc, path);
    if (task->m_http.m_path == NULL) {
    }
    
    return task;
}

void net_ping_task_free(net_ping_task_t task) {
    net_ping_mgr_t mgr = task->m_mgr;

    while(!TAILQ_EMPTY(&task->m_records)) {
        net_ping_record_free(TAILQ_FIRST(&task->m_records));
    }
    
    if (task->m_processor) {
        net_ping_processor_free(task->m_processor);
        task->m_processor = NULL;
    }

    switch(task->m_type) {
    case net_ping_type_http:
        if (task->m_http.m_path) {
            mem_free(mgr->m_alloc, task->m_http.m_path);
            task->m_http.m_path = NULL;
        }
        break;
    default:
        break;
    }
    
    TAILQ_INSERT_TAIL(&mgr->m_free_tasks, task, m_next);
}

void net_ping_task_real_free(net_ping_task_t task) {
    net_ping_mgr_t mgr = task->m_mgr;
    TAILQ_REMOVE(&mgr->m_free_tasks, task, m_next);
    mem_free(mgr->m_alloc, task);
}

net_address_t net_ping_task_target(net_ping_task_t task) {
    return task->m_target;
}

net_ping_task_state_t net_ping_task_state(net_ping_task_t task) {
    return task->m_state;
}

static net_ping_record_t net_ping_task_record_next(net_ping_record_it_t it) {
    net_ping_record_t * data = (net_ping_record_t *)it->data;

    net_ping_record_t r = *data;

    if (r) {
        *data = TAILQ_NEXT(r, m_next);
    }

    return r;
}

void net_ping_task_records(net_ping_task_t task, net_ping_record_it_t record_it) {
    *(net_ping_record_t *)record_it->data = TAILQ_FIRST(&task->m_records);
    record_it->next = net_ping_task_record_next;
}

int net_ping_task_start(net_ping_task_t task, uint16_t ping_count) {
    net_ping_mgr_t mgr = task->m_mgr;
    assert(ping_count > 0);

    if (task->m_processor) {
        net_ping_processor_free(task->m_processor);
        task->m_processor = NULL;
    }

    net_ping_task_set_state(task, net_ping_task_state_processing);
    
    task->m_processor = net_ping_processor_create(task, ping_count);
    if (task->m_processor == NULL) {
        net_ping_task_set_state(task, net_ping_task_state_error);
        return -1;
    }

    int rv = 0;
    
    switch(task->m_type) {
    case net_ping_type_icmp:
        rv = net_ping_processor_start_icmp(task->m_processor);
        break;
    case net_ping_type_tcp_connect:
        rv = net_ping_processor_start_tcp_connect(task->m_processor);
        break;
    case net_ping_type_http:
        rv = net_ping_processor_start_http(task->m_processor);
        break;
    }

    if (rv != 0) {
        net_ping_processor_free(task->m_processor);
        task->m_processor = NULL;
        net_ping_task_set_state(task, net_ping_task_state_error);
        return -1;
    }

    return 0;
}

uint32_t net_ping_task_ping_max(net_ping_task_t task) {
    uint32_t max_ping = 0;

    net_ping_record_t record;
    TAILQ_FOREACH(record, &task->m_records, m_next) {
        if (record->m_value > max_ping) {
            max_ping = record->m_value;
        }
    }
    
    return max_ping;
}

uint32_t net_ping_task_ping_min(net_ping_task_t task) {
    uint32_t min_ping = 0;

    net_ping_record_t record;
    TAILQ_FOREACH(record, &task->m_records, m_next) {
        if (record->m_value < min_ping) {
            min_ping = record->m_value;
        }
    }
    
    return min_ping;
}

uint32_t net_ping_task_ping_avg(net_ping_task_t task) {
    uint32_t total_ping = 0;
    uint32_t count = 0;

    net_ping_record_t record;
    TAILQ_FOREACH(record, &task->m_records, m_next) {
        total_ping += record->m_value;
        count++;
    }
    
    return count > 0 ? (uint32_t)(total_ping / count) : 0;
}


void net_ping_task_print(write_stream_t ws, net_ping_task_t task) {
    net_address_print(ws, task->m_target);
}

const char * net_ping_task_dump(mem_buffer_t buffer, net_ping_task_t task) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

    mem_buffer_clear_data(buffer);
    net_ping_task_print((write_stream_t)&stream, task);
    stream_putc((write_stream_t)&stream, 0);

    return mem_buffer_make_continuous(buffer, 0);
}

void net_ping_task_set_state(net_ping_task_t task, net_ping_task_state_t state) {
    if (task->m_state == state) return;

    net_ping_mgr_t mgr = task->m_mgr;
    
    CPE_INFO(
        mgr->m_em, "ping: task: state %s ==> %s", 
        net_ping_task_state_str(task->m_state), net_ping_task_state_str(state));
}

const char * net_ping_task_state_str(net_ping_task_state_t state) {
    switch(state) {
    case net_ping_task_state_init:
        return "init";
    case net_ping_task_state_processing:
        return "processing";
    case net_ping_task_state_done:
        return "done";
    case net_ping_task_state_error:
        return "error";
    }
}
