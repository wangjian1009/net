#include <assert.h>
#include "net_log_thread_i.h"
#include "net_log_request.h"
#include "net_log_request_cache.h"

void net_log_thread_process_cmd_send(
    net_log_thread_t log_thread, net_log_request_param_t send_param)
{
    ASSERT_ON_THREAD(log_thread);
    
    assert(send_param);

    net_log_request_cache_t cache = TAILQ_FIRST(&log_thread->m_caches);
    if (cache == NULL) { /*没有使用中的缓存 */
        if (log_thread->m_request_buf_size < log_thread->m_schedule->m_cfg_cache_mem_capacity) { /*没有超过内存限制 */
            net_log_request_t request = net_log_request_create(log_thread, send_param);
            if (request == NULL) {
                CPE_ERROR(log_thread->m_em, "log: thread %s: send: create request fail", log_thread->m_name);
                goto SEND_FAIL;
            }

            net_log_thread_check_active_requests(log_thread);
            return;
        }
    }

    if (cache == NULL || cache->m_state != net_log_request_cache_building) {
        cache = net_log_request_cache_create(log_thread, log_thread->m_cache_max_id + 1, net_log_request_cache_building);
        if (cache == NULL) {
            CPE_ERROR(log_thread->m_em, "log: thread %s: send: create cache fail", log_thread->m_name);
            goto SEND_FAIL;
        }
        log_thread->m_cache_max_id++;
    }

    if (net_log_request_cache_append(cache, send_param) != 0) {
        CPE_ERROR(log_thread->m_em, "log: thread %s: send: append to cache %d fail", log_thread->m_name, cache->m_id);
        goto SEND_FAIL;
    }

    if (cache->m_size >= log_thread->m_schedule->m_cfg_cache_file_capacity) {
        net_log_request_cache_close(cache);
    }

    net_log_request_param_free(send_param);
    return;
    
SEND_FAIL:
    //TODO: Loki net_log_category_add_fail_statistics(send_param->category, send_param->log_count);
    net_log_request_param_free(send_param);
}


void net_log_thread_process_cmd_pause(net_log_thread_t log_thread) {
    ASSERT_ON_THREAD(log_thread);
    
    net_log_schedule_t schedule = log_thread->m_schedule;
    
    if (log_thread->m_state == net_log_thread_state_pause) {
        if (schedule->m_debug) {
            CPE_INFO(log_thread->m_em, "log: thread %s: already pause", log_thread->m_name);
        }
        return;
    }
    
    net_log_thread_set_state(log_thread, net_log_thread_state_pause);
}

void net_log_thread_process_cmd_resume(net_log_thread_t log_thread) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD(log_thread);

    /* if (log_thread->m_state == net_log_thread_state_runing) { */
    /*     if (schedule->m_debug) { */
    /*         CPE_INFO(log_thread->m_em, "log: thread %s: already running", log_thread->m_name); */
    /*     } */
    /*     return; */
    /* } */

    //net_log_thread_set_state(log_thread, net_log_thread_state_runing);

    //net_log_request_log_thread_check_active_requests(log_thread);    
}

void net_log_thread_process_cmd_stop_begin(net_log_thread_t log_thread) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD(log_thread);

    if (net_log_thread_request_is_empty(log_thread)) {
        net_log_thread_process_cmd_stop_force(log_thread);
    }
    else {
        //net_log_thread_set_state(log_thread, net_log_thread_state_stoping);
        if (schedule->m_debug) {
            CPE_INFO(log_thread->m_em, "log: thread %s: stoping: wait all request done", log_thread->m_name);
        }
    }
}

void net_log_thread_process_cmd_stop_force(net_log_thread_t log_thread) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD(log_thread);

    //if (log_thread->m_state == net_log_thread_state_stoped) return;
    
    //net_log_thread_set_state(log_thread, net_log_thread_state_stoped);
    CPE_ERROR(log_thread->m_em, "log: thread %s: manage: not support stop", log_thread->m_name);
}


