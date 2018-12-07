#include "assert.h"
#include "cpe/pal/pal_strings.h"
#include "net_schedule.h"
#include "net_address.h"
#include "net_timer.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_http_protocol.h"
#include "net_http_endpoint.h"
#include "net_trans_manage_i.h"
#include "net_trans_http_protocol_i.h"
#include "net_trans_http_endpoint_i.h"
#include "net_trans_task_i.h"
#include "net_trans_host_i.h"

net_trans_manage_t net_trans_manage_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule, net_driver_t driver)
{
    net_trans_manage_t manage = mem_alloc(alloc, sizeof(struct net_trans_manage));
    if (manage == NULL) {
        CPE_ERROR(em, "trans-cli: manage alloc fail");
        return NULL;
    }
    
    bzero(manage, sizeof(*manage));

    manage->m_alloc = alloc;
    manage->m_em = em;
    manage->m_debug = 0;
    manage->m_schedule = schedule;
    manage->m_driver = driver;
    manage->m_watcher_ctx = NULL;
    manage->m_watcher_fun = NULL;
    manage->m_cfg_host_endpoint_limit = 0;

    TAILQ_INIT(&manage->m_free_tasks);
    TAILQ_INIT(&manage->m_free_hosts);
    
    manage->m_http_protocol = net_trans_http_protocol_create(manage);
    if (manage->m_http_protocol == NULL) {
        mem_free(alloc, manage);
        return NULL;
    }

    if (cpe_hash_table_init(
            &manage->m_hosts,
            alloc,
            (cpe_hash_fun_t) net_trans_host_hash,
            (cpe_hash_eq_t) net_trans_host_eq,
            CPE_HASH_OBJ2ENTRY(net_trans_host, m_hh_for_mgr),
            -1) != 0)
    {
        net_trans_http_protocol_free(manage->m_http_protocol);
        mem_free(alloc, manage);
        return NULL;
    }
    
    if (cpe_hash_table_init(
            &manage->m_tasks,
            alloc,
            (cpe_hash_fun_t) net_trans_task_hash,
            (cpe_hash_eq_t) net_trans_task_eq,
            CPE_HASH_OBJ2ENTRY(net_trans_task, m_hh_for_mgr),
            -1) != 0)
    {
        cpe_hash_table_fini(&manage->m_hosts);
        net_trans_http_protocol_free(manage->m_http_protocol);
        mem_free(alloc, manage);
        return NULL;
    }
    
    return manage;
}

void net_trans_manage_free(net_trans_manage_t mgr) {
    net_trans_host_free_all(mgr);
    net_trans_task_free_all(mgr);
    cpe_hash_table_fini(&mgr->m_hosts);
    cpe_hash_table_fini(&mgr->m_tasks);
    
    mgr->m_watcher_ctx = NULL;
    mgr->m_watcher_fun = NULL;
    
    if (mgr->m_http_protocol) {
        net_trans_http_protocol_free(mgr->m_http_protocol);
        mgr->m_http_protocol = NULL;
    }

    while(!TAILQ_EMPTY(&mgr->m_free_tasks)) {
        net_trans_task_real_free(TAILQ_FIRST(&mgr->m_free_tasks));
    }

    while(!TAILQ_EMPTY(&mgr->m_free_hosts)) {
        net_trans_host_real_free(TAILQ_FIRST(&mgr->m_free_hosts));
    }
    
    mem_free(mgr->m_alloc, mgr);
}

uint8_t net_trans_manage_debug(net_trans_manage_t manage) {
    return manage->m_debug;
}

void net_trans_manage_set_debug(net_trans_manage_t manage, uint8_t debug) {
    manage->m_debug = debug;
}

void net_trans_manage_set_data_watcher(
    net_trans_manage_t mgr,
    void * watcher_ctx,
    net_endpoint_data_watch_fun_t watcher_fun)
{
    mgr->m_watcher_ctx = watcher_ctx;
    mgr->m_watcher_fun = watcher_fun;
}

mem_buffer_t net_trans_manage_tmp_buffer(net_trans_manage_t manage) {
    return net_schedule_tmp_buffer(manage->m_schedule);
}
