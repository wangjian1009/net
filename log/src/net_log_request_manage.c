#include "assert.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/tailq_sort.h"
#include "cpe/vfs/vfs_dir.h"
#include "cpe/vfs/vfs_entry_info.h"
#include "net_timer.h"
#include "net_log_request_manage.h"
#include "net_log_request.h"
#include "net_log_request_cmd.h"
#include "net_log_category_i.h"
#include "net_log_request_cache.h"

net_log_request_manage_t
net_log_request_manage_create(
    net_log_schedule_t schedule, net_schedule_t net_schedule, net_driver_t net_driver,
    uint8_t max_active_request_count, const char * name, mem_buffer_t tmp_buffer,
    void (*stop_fun)(void * ctx), void * stop_ctx)
{
    net_log_request_manage_t mgr = mem_alloc(schedule->m_alloc, sizeof(struct net_log_request_manage));

    mgr->m_schedule = schedule;
    mgr->m_net_schedule = net_schedule;
    mgr->m_net_driver = net_driver;
    mgr->m_state = net_log_request_manage_state_runing;
    mgr->m_cfg_active_request_count = max_active_request_count;
    mgr->m_stop_fun = stop_fun;
    mgr->m_stop_ctx = stop_ctx;
    mgr->m_timer_event = net_timer_auto_create(net_schedule, net_log_request_manage_do_timeout, mgr);

	mgr->m_multi_handle = curl_multi_init();
    mgr->m_still_running = 0;
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

    curl_multi_setopt(mgr->m_multi_handle, CURLMOPT_SOCKETFUNCTION, net_log_request_manage_sock_cb);
    curl_multi_setopt(mgr->m_multi_handle, CURLMOPT_SOCKETDATA, mgr);
    curl_multi_setopt(mgr->m_multi_handle, CURLMOPT_TIMERFUNCTION, net_log_request_manage_timer_cb);
    curl_multi_setopt(mgr->m_multi_handle, CURLMOPT_TIMERDATA, mgr);
    
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

    mem_free(schedule->m_alloc, mgr);
}

void net_log_request_manage_active_next(net_log_request_manage_t mgr) {
    net_log_request_t request = TAILQ_FIRST(&mgr->m_waiting_requests);
    if (request) {
        net_log_request_active(request);
    }
}

static void net_log_request_manage_process_cmd_send(
    net_log_schedule_t schedule, net_log_request_manage_t mgr, net_log_request_param_t send_param)
{
    assert(send_param);

    net_log_request_cache_t cache = TAILQ_FIRST(&mgr->m_caches);
    if (cache) {
        if (net_log_request_cache_append(cache, send_param) != 0) {
            CPE_ERROR(schedule->m_em, "log: %s: cache: append fail", mgr->m_name);
            net_log_category_add_fail_statistics(send_param->category, send_param->log_count);
            net_log_request_param_free(send_param);
        }

        return;
    }

    net_log_request_t request = net_log_request_create(mgr, send_param);
    if (request == NULL) {
        CPE_ERROR(schedule->m_em, "log: %s: manage: create request fail", mgr->m_name);
        net_log_category_add_fail_statistics(send_param->category, send_param->log_count);
        net_log_request_param_free(send_param);
    }
    else {
        if (mgr->m_active_request_count < mgr->m_cfg_active_request_count) {
            net_log_request_active(request);
        }
        else {
            if (schedule->m_debug) {
                CPE_INFO(
                    schedule->m_em, "log: %s: manage: max active request %d reached, delay active",
                    mgr->m_name, mgr->m_cfg_active_request_count);
            }
        }
    }
}

static void net_log_request_manage_process_cmd_pause(net_log_schedule_t schedule, net_log_request_manage_t mgr) {
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

static void net_log_request_manage_process_cmd_resume(net_log_schedule_t schedule, net_log_request_manage_t mgr) {
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
}

static void net_log_request_manage_process_cmd_stop(net_log_schedule_t schedule, net_log_request_manage_t mgr) {
    if (mgr->m_stop_fun) {
        mgr->m_stop_fun(mgr->m_stop_ctx);
    }
    else {
        CPE_ERROR(schedule->m_em, "log: %s: manage: not support stop", mgr->m_name);
    }
}

void net_log_request_manage_process_cmd(
    net_log_request_manage_t mgr, net_log_request_cmd_t cmd, net_log_request_param_t send_param)
{
    net_log_schedule_t schedule = mgr->m_schedule;

    assert((cmd->m_cmd == net_log_request_cmd_send && send_param != NULL)
           ||
           (cmd->m_cmd != net_log_request_cmd_send && send_param == NULL));
           
    switch(cmd->m_cmd) {
    case net_log_request_cmd_send:
        net_log_request_manage_process_cmd_send(schedule, mgr, send_param);
        break;
    case net_log_request_cmd_pause:
        net_log_request_manage_process_cmd_pause(schedule, mgr);
        break;
    case net_log_request_cmd_resume:
        net_log_request_manage_process_cmd_resume(schedule, mgr);
        break;
    case net_log_request_cmd_stop:
        net_log_request_manage_process_cmd_stop(schedule, mgr);
        break;
    default:
        CPE_ERROR(schedule->m_em, "log: %s: manage: unknown cmd %d", mgr->m_name, cmd->m_cmd);
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

    if (mem_buffer_printf(tmp_buffer, "%s/%s/cache_%5d", schedule->m_cfg_cache_dir, mgr->m_name, id) < 0) {
        CPE_ERROR(schedule->m_em, "log: %s: manage: format buffer fail", mgr->m_name);
        return NULL;
    }

    return mem_buffer_make_continuous(tmp_buffer, 0);
}

int net_log_request_mgr_search_cache(net_log_request_manage_t mgr) {
    net_log_schedule_t schedule = mgr->m_schedule;

    const char * cache_dir = net_log_request_manage_cache_dir(mgr, mgr->m_tmp_buffer);
    if (cache_dir == NULL) {
        CPE_ERROR(schedule->m_em, "log: %s: manage: calc cache dir fail", mgr->m_name);
        return -1;
    }

    if (!vfs_dir_exist(schedule->m_vfs, cache_dir)) {
        if (schedule->m_debug) {
            CPE_INFO(
                schedule->m_em, "log: %s: manage: cache dir %s not exist, skip",
                mgr->m_name, cache_dir);
        }
        return 0;
    }
    
    vfs_dir_t d = vfs_dir_open(schedule->m_vfs, cache_dir);
    if (d == NULL) {
        CPE_ERROR(
            schedule->m_em, "log: %s: manage: cache dir %s open fail",
            mgr->m_name, cache_dir);
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
                schedule->m_em, "log: %s: manage: cache %s(id=%d) create fail",
                mgr->m_name, entry->m_name, id);
            rv = -1;
            continue;
        }
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
