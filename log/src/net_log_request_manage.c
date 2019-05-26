#include "assert.h"
#include "cpe/pal/pal_errno.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/tailq_sort.h"
#include "cpe/vfs/vfs_dir.h"
#include "cpe/vfs/vfs_entry_info.h"
#include "net_timer.h"
#include "net_trans_manage.h"
#include "net_log_request_manage.h"
#include "net_log_request.h"
#include "net_log_pipe_cmd.h"
#include "net_log_category_i.h"
#include "net_log_request_cache.h"

net_log_request_manage_t
net_log_request_manage_create(
    net_log_schedule_t schedule, net_schedule_t net_schedule, net_driver_t net_driver,
    uint16_t max_active_request_count, const char * name, mem_buffer_t tmp_buffer,
    void (*stop_fun)(void * ctx), void * stop_ctx)
{
    net_log_request_manage_t mgr = mem_alloc(schedule->m_alloc, sizeof(struct net_log_request_manage));

    mgr->m_schedule = schedule;
    mgr->m_net_schedule = net_schedule;
    mgr->m_net_driver = net_driver;

    mgr->m_trans_mgr = net_trans_manage_create(schedule->m_alloc, schedule->m_em, net_schedule, net_driver);
    if (mgr->m_trans_mgr == NULL) {
        CPE_ERROR(schedule->m_em, "log: %s: manage: create: create trans mgr fail", name);
        mem_free(schedule->m_alloc, mgr);
        return NULL;
    }

    mgr->m_state =
        net_log_schedule_state(schedule) == net_log_schedule_state_pause
        ? net_log_request_manage_state_pause
        : net_log_request_manage_state_runing;
    mgr->m_cfg_active_request_count = max_active_request_count;
    mgr->m_stop_fun = stop_fun;
    mgr->m_stop_ctx = stop_ctx;

    mgr->m_request_max_id = 0;
    mgr->m_request_count = 0;
    mgr->m_request_buf_size = 0;
    mgr->m_active_request_count = 0;
    mgr->m_cache_max_id = 0;
    mgr->m_name = name;
    mgr->m_tmp_buffer = tmp_buffer;

    TAILQ_INIT(&mgr->m_caches);
    TAILQ_INIT(&mgr->m_waiting_requests);
    TAILQ_INIT(&mgr->m_active_requests);
    TAILQ_INIT(&mgr->m_free_requests);
    
    return mgr;
}

void net_log_request_manage_free(net_log_request_manage_t mgr) {
    net_log_schedule_t schedule = mgr->m_schedule;

    while(!TAILQ_EMPTY(&mgr->m_waiting_requests)) {
        net_log_request_free(TAILQ_FIRST(&mgr->m_waiting_requests));
    }

    while(!TAILQ_EMPTY(&mgr->m_active_requests)) {
        net_log_request_free(TAILQ_FIRST(&mgr->m_active_requests));
    }
    
    while(!TAILQ_EMPTY(&mgr->m_free_requests)) {
        net_log_request_real_free(TAILQ_FIRST(&mgr->m_free_requests));
    }

    while(!TAILQ_EMPTY(&mgr->m_caches)) {
        net_log_request_cache_free(TAILQ_FIRST(&mgr->m_caches));
    }

    assert(mgr->m_trans_mgr);
    net_trans_manage_free(mgr->m_trans_mgr);
    mgr->m_trans_mgr = NULL;

    mem_free(schedule->m_alloc, mgr);
}

void net_log_request_manage_process_cmd_send(
    net_log_request_manage_t mgr, net_log_request_param_t send_param)
{
    net_log_schedule_t schedule = mgr->m_schedule;
    
    assert(send_param);

    net_log_request_cache_t cache = TAILQ_FIRST(&mgr->m_caches);
    if (cache == NULL) { /*没有使用中的缓存 */
        if (mgr->m_request_buf_size < schedule->m_cfg_cache_mem_capacity) { /*没有超过内存限制 */
            net_log_request_t request = net_log_request_create(mgr, send_param);
            if (request == NULL) {
                CPE_ERROR(schedule->m_em, "log: %s: manage: send: create request fail", mgr->m_name);
                goto SEND_FAIL;
            }

            net_log_request_mgr_check_active_requests(mgr);
            return;
        }
    }

    if (cache == NULL || cache->m_state != net_log_request_cache_building) {
        cache = net_log_request_cache_create(mgr, mgr->m_cache_max_id + 1, net_log_request_cache_building);
        if (cache == NULL) {
            CPE_ERROR(schedule->m_em, "log: %s: manage: send: create cache fail", mgr->m_name);
            goto SEND_FAIL;
        }
        mgr->m_cache_max_id++;
    }

    if (net_log_request_cache_append(cache, send_param) != 0) {
        CPE_ERROR(schedule->m_em, "log: %s: manage: send: append to cache %d fail", mgr->m_name, cache->m_id);
        goto SEND_FAIL;
    }

    if (cache->m_size >= schedule->m_cfg_cache_file_capacity) {
        net_log_request_cache_close(cache);
    }

    net_log_request_param_free(send_param);
    return;
    
SEND_FAIL:
    net_log_category_add_fail_statistics(send_param->category, send_param->log_count);
    net_log_request_param_free(send_param);
}

void net_log_request_manage_process_cmd_pause(net_log_request_manage_t mgr) {
    net_log_schedule_t schedule = mgr->m_schedule;
    
    if (mgr->m_state == net_log_request_manage_state_pause) {
        if (schedule->m_debug) {
            CPE_INFO(schedule->m_em, "log: %s: manage: already pause", mgr->m_name);
        }
        return;
    }

    mgr->m_state = net_log_request_manage_state_pause;
    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: %s: manage: pause", mgr->m_name);
    }
}

void net_log_request_manage_process_cmd_resume(net_log_request_manage_t mgr) {
    net_log_schedule_t schedule = mgr->m_schedule;

    if (mgr->m_state == net_log_request_manage_state_runing) {
        if (schedule->m_debug) {
            CPE_INFO(schedule->m_em, "log: %s: manage: already running", mgr->m_name);
        }
        return;
    }

    mgr->m_state = net_log_request_manage_state_runing;
    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: %s: manage: running", mgr->m_name);
    }

    net_log_request_mgr_check_active_requests(mgr);    
}

void net_log_request_manage_process_cmd_stop_begin(net_log_request_manage_t mgr) {
    net_log_schedule_t schedule = mgr->m_schedule;

    if (net_log_request_mgr_is_empty(mgr)) {
        net_log_request_manage_process_cmd_stop_complete(mgr);
    }
    else {
        mgr->m_state = net_log_request_manage_state_stoping;
        if (schedule->m_debug) {
            CPE_INFO(schedule->m_em, "log: %s: manage: stoping: wait all request done", mgr->m_name);
        }
    }
}

void net_log_request_manage_process_cmd_stop_complete(net_log_request_manage_t mgr) {
    net_log_schedule_t schedule = mgr->m_schedule;

    if (mgr->m_state == net_log_request_manage_state_stoped) return;
    
    mgr->m_state = net_log_request_manage_state_stoped;
    if (mgr->m_stop_fun) {
        mgr->m_stop_fun(mgr->m_stop_ctx);
    }
    else {
        CPE_ERROR(schedule->m_em, "log: %s: manage: not support stop", mgr->m_name);
    }
}

const char * net_log_request_manage_cache_dir(net_log_request_manage_t mgr, mem_buffer_t tmp_buffer) {
    net_log_schedule_t schedule = mgr->m_schedule;

    assert(schedule->m_cfg_cache_dir);
    
    mem_buffer_clear_data(tmp_buffer);

    if (mem_buffer_printf(tmp_buffer, "%s/%s", schedule->m_cfg_cache_dir, mgr->m_name) < 0) {
        CPE_ERROR(schedule->m_em, "log: %s: manage: format buffer fail", mgr->m_name);
        return NULL;
    }

    return mem_buffer_make_continuous(tmp_buffer, 0);
}

const char * net_log_request_manage_cache_file(net_log_request_manage_t mgr, uint32_t id, mem_buffer_t tmp_buffer) {
    net_log_schedule_t schedule = mgr->m_schedule;

    assert(schedule->m_cfg_cache_dir);
    
    mem_buffer_clear_data(tmp_buffer);

    if (mem_buffer_printf(tmp_buffer, "%s/%s/cache_%05d", schedule->m_cfg_cache_dir, mgr->m_name, id) < 0) {
        CPE_ERROR(schedule->m_em, "log: %s: manage: format buffer fail", mgr->m_name);
        return NULL;
    }

    return mem_buffer_make_continuous(tmp_buffer, 0);
}

int net_log_request_mgr_search_cache(net_log_request_manage_t mgr) {
    net_log_schedule_t schedule = mgr->m_schedule;

    const char * cache_dir = net_log_request_manage_cache_dir(mgr, mgr->m_tmp_buffer);
    if (cache_dir == NULL) {
        CPE_ERROR(schedule->m_em, "log: %s: manage: search cache: calc cache dir fail", mgr->m_name);
        return -1;
    }

    if (!vfs_dir_exist(schedule->m_vfs, cache_dir)) {
        if (schedule->m_debug) {
            CPE_INFO(
                schedule->m_em, "log: %s: manage: search cache: cache dir %s not exist, skip",
                mgr->m_name, cache_dir);
        }
        return 0;
    }
    
    vfs_dir_t d = vfs_dir_open(schedule->m_vfs, cache_dir);
    if (d == NULL) {
        CPE_ERROR(
            schedule->m_em, "log: %s: manage: search cache: cache dir %s open fail, error=%d (%s)",
            mgr->m_name, cache_dir, errno, strerror(errno));
        return -1;
    }

    struct vfs_entry_info_it entry_it;
    vfs_dir_entries(d, &entry_it);

    int rv = 0;

    vfs_entry_info_t entry;
    while((entry = vfs_entry_info_it_next(&entry_it))) {
        if (entry->m_type != vfs_entry_file) continue;
        if (!cpe_str_start_with(entry->m_name, "cache_")) continue;

        const char * str_id = entry->m_name + 6;
        uint32_t id = atoi(str_id);

        net_log_request_cache_t cache = net_log_request_cache_create(mgr, id, net_log_request_cache_done);
        if (cache == NULL) {
            CPE_ERROR(
                schedule->m_em, "log: %s: manage: search cache: cache %s(id=%d) create fail",
                mgr->m_name, entry->m_name, id);
            rv = -1;
            continue;
        }

        if (id > mgr->m_cache_max_id) mgr->m_cache_max_id = id;
    }

    vfs_dir_close(d);

    TAILQ_SORT(&mgr->m_caches, net_log_request_cache, net_log_request_cache_list, m_next, net_log_request_cache_cmp, NULL);

    if (schedule->m_debug) {
        net_log_request_cache_t cache;
        TAILQ_FOREACH(cache, &mgr->m_caches, m_next) {
            CPE_INFO(schedule->m_em, "log: %s: manage: found cache %d", mgr->m_name, cache->m_id);
        }
    }

    return rv;
}

void net_log_request_mgr_check_active_requests(net_log_request_manage_t mgr) {
    net_log_schedule_t schedule = mgr->m_schedule;

    if (mgr->m_state == net_log_request_manage_state_pause
        || mgr->m_state == net_log_request_manage_state_stoped) return;

    while(mgr->m_cfg_active_request_count == 0
          || mgr->m_active_request_count < mgr->m_cfg_active_request_count)
    {
        net_log_request_t request = TAILQ_FIRST(&mgr->m_waiting_requests);
        if (request) {
            net_log_request_active(request);
        }
        else {
            net_log_request_cache_t cache;
            while((cache = TAILQ_LAST(&mgr->m_caches, net_log_request_cache_list))) {
                if (net_log_request_cache_load(cache) != 0) {
                    CPE_ERROR(schedule->m_em, "log: %s: restore: cache %d load fail, clear", mgr->m_name, cache->m_id);
                    net_log_request_cache_clear_and_free(cache);
                    continue;
                }

                net_log_request_cache_clear_and_free(cache);

                if (!TAILQ_EMPTY(&mgr->m_waiting_requests)) {
                    if (schedule->m_debug) {
                        CPE_INFO(
                            schedule->m_em, "log: %s: restore: load %d requests",
                            mgr->m_name, (mgr->m_request_count - mgr->m_active_request_count));
                    }
                    break;
                }
            }

            if (TAILQ_EMPTY(&mgr->m_waiting_requests)) break;
        }
    }

    if (mgr->m_state == net_log_request_manage_state_stoping) {
        if (net_log_request_mgr_is_empty(mgr)) {
            if (schedule->m_debug) {
                CPE_INFO(schedule->m_em, "log: %s: manage: stoping: all request done!", mgr->m_name);
            }
            net_log_request_manage_process_cmd_stop_complete(mgr);
        }
    }
}

int net_log_request_mgr_save_and_clear_requests(net_log_request_manage_t mgr) {
    net_log_schedule_t schedule = mgr->m_schedule;
    
    if(TAILQ_EMPTY(&mgr->m_active_requests) && TAILQ_EMPTY(&mgr->m_waiting_requests)) {
        if (schedule->m_debug) {
            CPE_INFO(schedule->m_em, "log: %s: manage: sanve and clear: no requests", mgr->m_name);
        }
        return 0;
    }
        
    net_log_request_cache_t next_cache = TAILQ_FIRST(&mgr->m_caches);
    assert(next_cache == NULL || next_cache->m_id > 0);
    
    uint32_t cache_id = next_cache ? (next_cache->m_id - 1) : (mgr->m_cache_max_id + 1);

    net_log_request_cache_t cache = net_log_request_cache_create(mgr, cache_id, net_log_request_cache_building);
    if (cache == NULL) {
        CPE_ERROR(schedule->m_em, "log: %s: manage: sanve and clear: create cache fail", mgr->m_name);
        return -1;
    }
    if (cache_id > mgr->m_cache_max_id) mgr->m_cache_max_id = cache_id;

    uint32_t count = 0;
    while(!TAILQ_EMPTY(&mgr->m_active_requests)) {
        net_log_request_t request = TAILQ_FIRST(&mgr->m_active_requests);
        if (net_log_request_cache_append(cache, request->m_send_param) != 0) {
            return -1;
        }
        count++;
        net_log_request_free(request);
    }

    while(!TAILQ_EMPTY(&mgr->m_waiting_requests)) {
        net_log_request_t request = TAILQ_FIRST(&mgr->m_waiting_requests);
        if (net_log_request_cache_append(cache, request->m_send_param) != 0) {
            return -1;
        }
        count++;
        net_log_request_free(request);
    }

    if (net_log_request_cache_close(cache) != 0) return -1;

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: %s: manage: sanve and clear: saved %d requests", mgr->m_name, count);
    }
    
    return 0;
}

int net_log_request_mgr_init_cache_dir(net_log_request_manage_t mgr) {
    net_log_schedule_t schedule = mgr->m_schedule;

    const char * dir = net_log_request_manage_cache_dir(mgr, mgr->m_tmp_buffer);
    if (dir == NULL) {
        CPE_ERROR(schedule->m_em, "log: %s: manage: init cache dir: calc dir fail", mgr->m_name);
        return -1;
    }

    if (vfs_dir_mk_recursion(schedule->m_vfs, dir) != 0) {
        CPE_ERROR(
            schedule->m_em, "log: %s: manage: init cache dir: mk dir fail, error=%d (%s)",
            mgr->m_name, errno, strerror(errno));
        return -1;
    }

    return 0;
}

uint8_t net_log_request_mgr_is_empty(net_log_request_manage_t mgr) {
    return (mgr->m_request_count == 0 && TAILQ_EMPTY(&mgr->m_caches))
        ? 1
        : 0;
}

