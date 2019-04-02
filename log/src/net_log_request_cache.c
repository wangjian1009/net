#include "net_log_request_cache.h"

net_log_request_cache_t
net_log_request_cache_create(net_log_request_manage_t mgr) {
    net_log_schedule_t schedule = mgr->m_schedule;

    net_log_request_cache_t cache = mem_alloc(schedule->m_alloc, sizeof(struct net_log_request_cache));
    if (cache == NULL) {
        CPE_ERROR(schedule->m_em, "log: %s: cache: alloc fail", mgr->m_name);
        return NULL;
    }

    cache->m_mgr = mgr;

    TAILQ_INSERT_TAIL(&mgr->m_caches, cache, m_next);
    return cache;
}

void net_log_request_cache_free(net_log_request_cache_t cache) {
    net_log_request_manage_t mgr = cache->m_mgr;
    net_log_schedule_t schedule = mgr->m_schedule;

    TAILQ_REMOVE(&mgr->m_caches, cache, m_next);
    
    mem_free(schedule->m_alloc, cache);
}

int net_log_request_cache_push(net_log_request_cache_t cache, net_log_request_param_t send_param) {
    return 0;
}
