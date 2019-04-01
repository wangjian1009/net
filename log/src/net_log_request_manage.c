#include "assert.h"
#include "net_timer.h"
#include "net_log_request_manage.h"
#include "net_log_request.h"
#include "net_log_request_cmd.h"

net_log_request_manage_t
net_log_request_manage_create(
    net_log_schedule_t schedule, net_schedule_t net_schedule, net_driver_t net_driver,
    uint8_t max_active_request_count, const char * name, mem_buffer_t tmp_buffer)
{
    net_log_request_manage_t mgr = mem_alloc(schedule->m_alloc, sizeof(struct net_log_request_manage));

    mgr->m_schedule = schedule;
    mgr->m_net_schedule = net_schedule;
    mgr->m_net_driver = net_driver;
    mgr->m_cfg_max_active_request_count = max_active_request_count;
    
    mgr->m_timer_event = net_timer_auto_create(net_schedule, net_log_request_manage_do_timeout, mgr);

	mgr->m_multi_handle = curl_multi_init();
    mgr->m_still_running = 0;
    mgr->m_request_max_id = 0;
    mgr->m_request_count = 0;
    mgr->m_active_request_count = 0;

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

    mem_free(schedule->m_alloc, mgr);
}

void net_log_request_manage_process_cmd(
    net_log_request_manage_t mgr, net_log_request_cmd_t cmd, net_log_request_param_t send_param)
{
    net_log_schedule_t schedule = mgr->m_schedule;

    assert((cmd->m_cmd == net_log_request_cmd_send && send_param != NULL)
           ||
           (cmd->m_cmd != net_log_request_cmd_send && send_param == NULL));
           
    if (cmd->m_cmd == net_log_request_cmd_send) {
        assert(send_param);

        net_log_request_t request = net_log_request_create(mgr, send_param);
        if (request == NULL) {
            CPE_ERROR(
                schedule->m_em, "log: %s: manage: create request fail %d", mgr->m_name, cmd->m_cmd);
            net_log_request_param_free(send_param);
        }

        
    }
    else {
        CPE_ERROR(
            schedule->m_em, "log: %s: manage: unknown cmd %d", mgr->m_name, cmd->m_cmd);
    }
}
