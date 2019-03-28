#include "net_timer.h"
#include "net_log_request_manage.h"
#include "net_log_request.h"

net_log_request_manage_t
net_log_request_manage_create(net_log_schedule_t schedule, net_schedule_t net_schedule, net_driver_t net_driver) {
    net_log_request_manage_t mgr = mem_alloc(schedule->m_alloc, sizeof(struct net_log_request_manage));

    mgr->m_schedule = schedule;
    mgr->m_net_schedule = net_schedule;
    mgr->m_net_driver = net_driver;

    mgr->m_timer_event = net_timer_auto_create(net_schedule, net_log_request_manage_do_timeout, mgr);

	mgr->m_multi_handle = curl_multi_init();
    mgr->m_still_running = 0;

    TAILQ_INIT(&mgr->m_requests);
    TAILQ_INIT(&mgr->m_free_requests);

    curl_multi_setopt(mgr->m_multi_handle, CURLMOPT_SOCKETFUNCTION, net_log_request_manage_sock_cb);
    curl_multi_setopt(mgr->m_multi_handle, CURLMOPT_SOCKETDATA, mgr);
    curl_multi_setopt(mgr->m_multi_handle, CURLMOPT_TIMERFUNCTION, net_log_request_manage_timer_cb);
    curl_multi_setopt(mgr->m_multi_handle, CURLMOPT_TIMERDATA, mgr);
    
    return mgr;
}

void net_log_request_manage_free(net_log_request_manage_t mgr) {
    net_log_schedule_t schedule = mgr->m_schedule;

    while(!TAILQ_EMPTY(&mgr->m_requests)) {
        net_log_request_free(TAILQ_FIRST(&mgr->m_requests));
    }

    while(!TAILQ_EMPTY(&mgr->m_free_requests)) {
        net_log_request_real_free(TAILQ_FIRST(&mgr->m_free_requests));
    }

    mem_free(schedule->m_alloc, mgr);
}
