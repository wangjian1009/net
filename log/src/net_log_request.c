#include "net_log_request.h"

net_log_request_t net_log_request_create(net_log_request_manage_t mgr) {
    net_log_schedule_t schedule = mgr->m_schedule;
    net_log_request_t request = TAILQ_FIRST(&mgr->m_requests);

    if (request) {
        TAILQ_REMOVE(&mgr->m_free_requests, request, m_next);
    }
    else {
        request = mem_alloc(schedule->m_alloc, sizeof(struct net_log_request));
        if (request == NULL) {
            CPE_ERROR(schedule->m_em, "log: request: alloc fail!");
            return NULL;
        }
    }

    request->m_mgr = mgr;

    TAILQ_INSERT_TAIL(&mgr->m_requests, request, m_next);

    return request;
}

void net_log_request_free(net_log_request_t request) {
    net_log_request_manage_t mgr = request->m_mgr;

    TAILQ_REMOVE(&mgr->m_requests, request, m_next);
    TAILQ_INSERT_TAIL(&mgr->m_free_requests, request, m_next);
}

void net_log_request_real_free(net_log_request_t request) {
    net_log_request_manage_t mgr = request->m_mgr;
    
    TAILQ_REMOVE(&mgr->m_free_requests, request, m_next);
    mem_free(mgr->m_schedule->m_alloc, request);
}

