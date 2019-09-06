#include <assert.h>
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/time_utils.h"
#include "net_schedule.h"
#include "net_timer.h"
#include "net_watcher.h"
#include "net_trans_task.h"
#include "net_ping_processor_i.h"
#include "net_ping_record_i.h"

static void net_ping_processor_timeout(net_timer_t timer, void * ctx);

net_ping_processor_t
net_ping_processor_create(net_ping_task_t task, uint32_t ping_span_ms, uint16_t ping_count) {
    net_ping_mgr_t mgr = task->m_mgr;
    
    net_ping_processor_t processor = TAILQ_FIRST(&mgr->m_free_processors);
    if (processor) {
        TAILQ_REMOVE(&mgr->m_free_processors, processor, m_next);
    }
    else {
        processor = mem_alloc(mgr->m_alloc, sizeof(struct net_ping_processor));
        if (processor == NULL) {
            CPE_ERROR(
                mgr->m_em, "ping: %s: processor: alloc fail!", 
                net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task));
            return NULL;
        }
    }

    processor->m_task = task;
    processor->m_timer = net_timer_auto_create(mgr->m_schedule, net_ping_processor_timeout, processor);
    if (processor->m_timer == NULL) {
        CPE_ERROR(
            mgr->m_em, "ping: %s: processor: create timer fail!", 
            net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task));
        processor->m_task = (net_ping_task_t)mgr;
        TAILQ_INSERT_TAIL(&mgr->m_free_processors, processor, m_next);
        return NULL;
    }
    
    switch(task->m_type) {
    case net_ping_type_icmp:
        processor->m_icmp.m_ping_id = ++mgr->m_icmp_ping_id_max;
        processor->m_icmp.m_fd = -1;
        processor->m_icmp.m_watcher = NULL;
        break;
    case net_ping_type_tcp_connect:
        processor->m_tcp_connect.m_fd = -1;
        processor->m_tcp_connect.m_watcher = NULL;
        break;
    case net_ping_type_http:
        processor->m_http.m_task = NULL;
        break;
    }
    processor->m_start_time_ms = 0;
    processor->m_ping_span_ms = ping_span_ms;
    processor->m_ping_count = ping_count;
    
    return processor;
}

void net_ping_processor_free(net_ping_processor_t processor) {
    net_ping_task_t task = processor->m_task;
    net_ping_mgr_t mgr = processor->m_task->m_mgr;

    switch(task->m_type) {
    case net_ping_type_icmp:
        if (processor->m_icmp.m_watcher) {
            net_watcher_free(processor->m_icmp.m_watcher);
            processor->m_icmp.m_watcher = NULL;
        }

        if (processor->m_icmp.m_fd != -1) {
            cpe_sock_close(processor->m_icmp.m_fd);
            processor->m_icmp.m_fd = -1;
        }
        break;
    case net_ping_type_tcp_connect:
        if (processor->m_tcp_connect.m_watcher) {
            net_watcher_free(processor->m_tcp_connect.m_watcher);
            processor->m_tcp_connect.m_watcher = NULL;
        }

        if (processor->m_tcp_connect.m_fd != -1) {
            cpe_sock_close(processor->m_tcp_connect.m_fd);
            processor->m_tcp_connect.m_fd = -1;
        }
        break;
    case net_ping_type_http:
        if (processor->m_http.m_task) {
            net_trans_task_clear_callback(processor->m_http.m_task);
            net_trans_task_free(processor->m_http.m_task);
            processor->m_http.m_task = NULL;
        }
        break;
    }

    if (processor->m_timer) {
        net_timer_free(processor->m_timer);
        processor->m_timer = NULL;
    }
    
    processor->m_task = (net_ping_task_t)mgr;
    TAILQ_INSERT_TAIL(&mgr->m_free_processors, processor, m_next);
}

void net_ping_processor_real_free(net_ping_processor_t processor) {
    net_ping_mgr_t mgr = (net_ping_mgr_t)processor->m_task;

    TAILQ_REMOVE(&mgr->m_free_processors, processor, m_next);
    
    mem_free(mgr->m_alloc, processor);
}

int net_ping_processor_start(net_ping_processor_t processor) {
    net_ping_task_t task = processor->m_task;
    net_ping_mgr_t mgr = task->m_mgr;

    int rv = 0;
    
    processor->m_start_time_ms = cur_time_ms();
    
    if (net_schedule_local_ip_stack(mgr->m_schedule) == net_local_ip_stack_none) {
        if (task->m_debug) {
            CPE_INFO(mgr->m_em, "ping: %s: start: no local networ, error!", net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task));
        }

        net_point_processor_set_result_one(processor, net_ping_error_no_network, "no-local-network", 0, 0, 0, 0);
        return -1;
    }
    
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

    return rv;
}

void net_point_processor_set_result_one(
    net_ping_processor_t processor, 
    net_ping_error_t error, const char * error_msg, uint8_t need_retry,
    uint32_t bytes, uint32_t ttl, uint32_t value)
{
    net_ping_task_t task = processor->m_task;
    int64_t ct_ms = cur_time_ms();
    
    if (value == 0) {
        value = ct_ms > processor->m_start_time_ms ? (uint32_t)(ct_ms - processor->m_start_time_ms) : 0;
    }
    
    net_ping_record_t record = net_ping_record_create(task, error, error_msg, bytes, ttl, value);
    if (record == NULL) {
        net_ping_task_set_state(task, net_ping_task_state_error);
        return;
    }

    if (need_retry && task->m_record_count < processor->m_ping_count) {
        /*需要下一次执行 */
        uint32_t used_duration = ct_ms > processor->m_start_time_ms ? (uint32_t)(ct_ms - processor->m_start_time_ms) : 0;
        if (used_duration < processor->m_ping_span_ms) {
            net_timer_active(processor->m_timer, processor->m_ping_span_ms - used_duration);
        }
        else {
            net_timer_active(processor->m_timer, 0);
        }
    }
    else {
        /*计算最终结果 */
        uint8_t success_count = 0;
        TAILQ_FOREACH(record, &task->m_records, m_next) {
            if (record->m_error == net_ping_error_none) success_count++;
        }
        
        net_ping_task_set_state(task, success_count ? net_ping_task_state_done : net_ping_task_state_error);
    }
}

static void net_ping_processor_timeout(net_timer_t timer, void * ctx) {
    net_ping_processor_t processor = ctx;
    net_ping_task_t task = processor->m_task;

    uint16_t record_count = task->m_record_count;
    int rv = net_ping_processor_start(task->m_processor);
    if (rv != 0) {
        if (task->m_record_count == record_count) {
            net_point_processor_set_result_one(task->m_processor, net_ping_error_internal, "retry-start-fail", 0, 0, 0, 0);
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
