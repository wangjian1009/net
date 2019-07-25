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
    task->m_record_count = 0;
    task->m_to_notify = 0;
    task->m_is_processing = 0;
    task->m_is_free = 0;
    task->m_cb_ctx = NULL;
    task->m_cb_fun = NULL;
    
    TAILQ_INIT(&task->m_records);

    task->m_target = net_address_copy(mgr->m_schedule, target);
    if (task->m_target == NULL) {
        CPE_ERROR(mgr->m_em, "ping: %s: dup target address fail!", net_address_dump(net_ping_mgr_tmp_buffer(mgr), target));
        TAILQ_INSERT_TAIL(&mgr->m_free_tasks, task, m_next);
        return NULL;
    }

    TAILQ_INSERT_TAIL(&mgr->m_tasks_no_notify, task, m_next);
    
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

net_ping_task_t net_ping_task_create_http(net_ping_mgr_t mgr, net_address_t target, uint8_t is_https, const char * path) {
    net_ping_task_t task = net_ping_task_create_i(mgr, target);
    task->m_type = net_ping_type_http;

    task->m_http.m_is_https = is_https;
    task->m_http.m_path = cpe_str_mem_dup(mgr->m_alloc, path);
    if (task->m_http.m_path == NULL) {
        CPE_ERROR(mgr->m_em, "ping: %s: dup target address fail!", net_address_dump(net_ping_mgr_tmp_buffer(mgr), target));
        net_ping_task_free(task);
        return NULL;
    }
    
    return task;
}

void net_ping_task_free(net_ping_task_t task) {
    net_ping_mgr_t mgr = task->m_mgr;

    if (task->m_is_processing) {
        task->m_is_free = 1;
        return;
    }
    
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

    if (task->m_to_notify) {
        TAILQ_REMOVE(&mgr->m_tasks_to_notify, task, m_next);
    }
    else {
        TAILQ_REMOVE(&mgr->m_tasks_no_notify, task, m_next);
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

net_ping_error_t net_ping_task_error(net_ping_task_t task) {
    net_ping_record_t record;
    TAILQ_FOREACH_REVERSE(record, &task->m_records, net_ping_record_list, m_next) {
        if (record->m_error != net_ping_error_no_network) {
            return record->m_error;
        }
    }
    return net_ping_error_none;
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

void net_ping_task_set_cb(net_ping_task_t task, void * cb_ctx, net_ping_task_cb_fun_t cb_fun) {
    task->m_cb_ctx = cb_ctx;
    task->m_cb_fun = cb_fun;
}

void net_ping_task_set_to_notify(net_ping_task_t task, uint8_t to_notify) {
    if (task->m_to_notify == to_notify) return;

    net_ping_mgr_t mgr = task->m_mgr;
    
    if (task->m_to_notify) {
        TAILQ_REMOVE(&mgr->m_tasks_to_notify, task, m_next);
    }
    else {
        TAILQ_REMOVE(&mgr->m_tasks_no_notify, task, m_next);
    }
    
    task->m_to_notify = to_notify;

    if (task->m_to_notify) {
        if (TAILQ_EMPTY(&mgr->m_tasks_to_notify)) {
            net_ping_mgr_active_delay_process(mgr);
        }
        TAILQ_INSERT_TAIL(&mgr->m_tasks_to_notify, task, m_next);
    }
    else {
        TAILQ_INSERT_TAIL(&mgr->m_tasks_no_notify, task, m_next);
    }
}

void net_ping_task_do_notify(net_ping_task_t task) {
    assert(!task->m_is_free);
    assert(!task->m_is_processing);
    
    task->m_is_processing = 1;
    
    net_ping_record_t to_notify_record = TAILQ_LAST(&task->m_records, net_ping_record_list);
    if (to_notify_record->m_to_notify) {
        net_ping_record_t pre_record;
        while((pre_record = TAILQ_PREV(to_notify_record, net_ping_record_list, m_next)) != NULL
              && pre_record->m_to_notify)
        {
            to_notify_record = pre_record;
        }
    }
    else {
        to_notify_record = NULL;
    }
    
    while(!task->m_is_free && to_notify_record) {
        to_notify_record->m_to_notify = 0;
        net_ping_record_t cur_reocrd = to_notify_record;
        to_notify_record = TAILQ_NEXT(to_notify_record, m_next);

        if (task->m_cb_fun) {
            task->m_cb_fun(task->m_cb_ctx, task, cur_reocrd);
        }
    }

    if (!task->m_is_free
        && (task->m_state == net_ping_task_state_error || task->m_state == net_ping_task_state_done)
        )
    {
        if (task->m_cb_fun) {
            task->m_cb_fun(task->m_cb_ctx, task, NULL);
        }
    }

    task->m_is_processing = 0;
    if (task->m_is_free) {
        net_ping_task_free(task);
        return;
    }
}

void net_ping_task_start(net_ping_task_t task, uint32_t ping_span_ms, uint16_t ping_count) {
    net_ping_mgr_t mgr = task->m_mgr;
    assert(ping_count > 0);

    if (task->m_processor) {
        net_ping_processor_free(task->m_processor);
        task->m_processor = NULL;
    }

    while(!TAILQ_EMPTY(&task->m_records)) {
        net_ping_record_free(TAILQ_FIRST(&task->m_records));
    }
    
    net_ping_task_set_state(task, net_ping_task_state_processing);
    
    task->m_processor = net_ping_processor_create(task, ping_span_ms, ping_count);
    if (task->m_processor == NULL) {
        net_ping_task_set_state(task, net_ping_task_state_error);
        return;
    }

    int rv = net_ping_processor_start(task->m_processor);
    if (rv != 0) {
        if (task->m_record_count == 0) {
            net_point_processor_set_result_one(task->m_processor, net_ping_error_internal, "start-fail", 0, 0, 0, 0);
            assert(task->m_state == net_ping_task_state_error);
        }
    }
    
    if (task->m_state == net_ping_task_state_error
        || task->m_state == net_ping_task_state_done)
    {
        assert(task->m_processor);
        net_ping_processor_free(task->m_processor);
        task->m_processor = NULL;
    }
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
        if (record->m_error == net_ping_error_no_network) {
            total_ping += record->m_value;
            count++;
        }
    }
    
    return count > 0 ? (uint32_t)(total_ping / count) : 0;
}

void net_ping_task_print(write_stream_t ws, net_ping_task_t task) {
    net_address_print(ws, task->m_target);
    
    stream_printf(ws, " use ");
    
    switch(task->m_type) {
    case net_ping_type_icmp:
        stream_printf(ws, "icmp");
        break;
    case net_ping_type_tcp_connect:
        stream_printf(ws, "tcp-connect");
        break;
    case net_ping_type_http:
        stream_printf(ws, "tcp-connect");
        break;
    }
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
        mgr->m_em, "ping: %s: state %s ==> %s", 
        net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task),
        net_ping_task_state_str(task->m_state), net_ping_task_state_str(state));
    
    task->m_state = state;

    if (state == net_ping_task_state_error || state == net_ping_task_state_done) {
        /*进入结束状态触发一次最终回调 */
        net_ping_task_set_to_notify(task, 1);
    }
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

const char * net_ping_error_str(net_ping_error_t err) {
    switch(err) {
    case net_ping_error_none:
        return "none";
    case net_ping_error_no_network:
        return "no-network";
    case net_ping_error_no_right:
        return "no-right";
    case net_ping_error_internal:
        return "internal";
    }
}
