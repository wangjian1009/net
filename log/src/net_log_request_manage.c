#include "net_log_request_manage.h"
#include "net_log_request.h"

net_log_request_manage_t
net_log_request_manage_create(net_log_schedule_t schedule, net_schedule_t net_schedule, net_driver_t net_driver) {
    net_log_request_manage_t mgr = mem_alloc(schedule->m_alloc, sizeof(struct net_log_request_manage));

    mgr->m_schedule = schedule;
    mgr->m_net_schedule = net_schedule;
    mgr->m_net_driver = net_driver;

    TAILQ_INIT(&mgr->m_requests);
    TAILQ_INIT(&mgr->m_free_requests);

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

