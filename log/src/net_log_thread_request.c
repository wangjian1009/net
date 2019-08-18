#include <assert.h>
#include "net_log_request.h"
#include "net_log_request_cache.h"
#include "net_log_thread_i.h"

uint8_t net_log_thread_request_is_empty(net_log_thread_t log_thread) {
    ASSERT_ON_THREAD(log_thread);
    
    return (log_thread->m_request_count == 0 && TAILQ_EMPTY(&log_thread->m_caches))
        ? 1
        : 0;
}

void net_log_thread_check_active_requests(net_log_thread_t log_thread) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD(log_thread);

    if (net_log_thread_is_suspend(log_thread)
        || log_thread->m_state == net_log_thread_state_stoped) return;

    while(log_thread->m_cfg_active_request_count == 0
          || log_thread->m_active_request_count < log_thread->m_cfg_active_request_count)
    {
        net_log_request_t request = TAILQ_FIRST(&log_thread->m_waiting_requests);
        if (request) {
            net_log_request_active(request);
        }
        else {
            net_log_request_cache_t cache;
            while((cache = TAILQ_LAST(&log_thread->m_caches, net_log_request_cache_list))) {
                if (net_log_request_cache_load(cache) != 0) {
                    CPE_ERROR(schedule->m_em, "log: %s: restore: cache %d load fail, clear", log_thread->m_name, cache->m_id);
                    net_log_request_cache_clear_and_free(cache);
                    continue;
                }

                net_log_request_cache_clear_and_free(cache);

                if (!TAILQ_EMPTY(&log_thread->m_waiting_requests)) {
                    if (schedule->m_debug) {
                        CPE_INFO(
                            schedule->m_em, "log: %s: restore: load %d requests",
                            log_thread->m_name, (log_thread->m_request_count - log_thread->m_active_request_count));
                    }
                    break;
                }
            }

            if (TAILQ_EMPTY(&log_thread->m_waiting_requests)) break;
        }
    }

    /* if (log_thread->m_state == net_log_thread_state_stoping) { */
    /*     if (net_log_thread_is_empty(log_thread)) { */
    /*         if (schedule->m_debug) { */
    /*             CPE_INFO(schedule->m_em, "log: %s: manage: stoping: all request done!", log_thread->m_name); */
    /*         } */
    /*         net_log_thread_process_cmd_stop_force(log_thread); */
    /*     } */
    /* } */
}

int net_log_thread_save_and_clear_requests(net_log_thread_t log_thread) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    
    if(TAILQ_EMPTY(&log_thread->m_active_requests) && TAILQ_EMPTY(&log_thread->m_waiting_requests)) {
        if (schedule->m_debug) {
            CPE_INFO(schedule->m_em, "log: %s: manage: sanve and clear: no requests", log_thread->m_name);
        }
        return 0;
    }
        
    net_log_request_cache_t next_cache = TAILQ_FIRST(&log_thread->m_caches);
    assert(next_cache == NULL || next_cache->m_id > 0);
    
    uint32_t cache_id = next_cache ? (next_cache->m_id - 1) : (log_thread->m_cache_max_id + 1);

    net_log_request_cache_t cache = net_log_request_cache_create(log_thread, cache_id, net_log_request_cache_building);
    if (cache == NULL) {
        CPE_ERROR(schedule->m_em, "log: %s: manage: sanve and clear: create cache fail", log_thread->m_name);
        return -1;
    }
    if (cache_id > log_thread->m_cache_max_id) log_thread->m_cache_max_id = cache_id;

    uint32_t count = 0;
    while(!TAILQ_EMPTY(&log_thread->m_active_requests)) {
        net_log_request_t request = TAILQ_FIRST(&log_thread->m_active_requests);
        if (net_log_request_cache_append(cache, request->m_send_param) != 0) {
            return -1;
        }
        count++;
        net_log_request_free(request);
    }

    while(!TAILQ_EMPTY(&log_thread->m_waiting_requests)) {
        net_log_request_t request = TAILQ_FIRST(&log_thread->m_waiting_requests);
        if (net_log_request_cache_append(cache, request->m_send_param) != 0) {
            return -1;
        }
        count++;
        net_log_request_free(request);
    }

    if (net_log_request_cache_close(cache) != 0) return -1;

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: %s: manage: sanve and clear: saved %d requests", log_thread->m_name, count);
    }
    
    return 0;
}
