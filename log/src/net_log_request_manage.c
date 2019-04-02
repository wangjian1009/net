#include "assert.h"
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
    mgr->m_active_request_count = 0;

    mgr->m_cache = NULL;
    mgr->m_name = name;
    mgr->m_tmp_buffer = tmp_buffer;

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

    if (mgr->m_cache) {
        net_log_request_cache_free(mgr->m_cache);
        mgr->m_cache = NULL;
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
