#include "assert.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/string_utils.h"
#include "net_schedule.h"
#include "net_address.h"
#include "net_timer.h"
#include "net_protocol.h"
#include "net_endpoint.h"
#include "net_trans_manage_i.h"
#include "net_trans_protocol_i.h"
#include "net_trans_endpoint_i.h"
#include "net_trans_task_i.h"

net_trans_manage_t net_trans_manage_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule, net_driver_t driver)
{
    net_trans_manage_t manage = mem_alloc(alloc, sizeof(struct net_trans_manage));
    if (manage == NULL) {
        CPE_ERROR(em, "trans-cli: manage alloc fail");
        return NULL;
    }
    
    manage->m_alloc = alloc;
    manage->m_em = em;
    manage->m_debug = 0;
    manage->m_schedule = schedule;
    manage->m_driver = driver;
    manage->m_watcher_ctx = NULL;
    manage->m_watcher_fun = NULL;
    manage->m_cfg_host_endpoint_limit = 0;

    manage->m_request_id_tag = NULL;
    manage->m_max_task_id = 0;

    TAILQ_INIT(&manage->m_free_tasks);
    
    manage->m_protocol = net_trans_protocol_create(manage);
    if (manage->m_protocol == NULL) {
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
        net_trans_protocol_free(manage->m_protocol);
        mem_free(alloc, manage);
        return NULL;
    }
    
    return manage;
}

void net_trans_manage_free(net_trans_manage_t mgr) {
    net_trans_task_free_all(mgr);
    cpe_hash_table_fini(&mgr->m_tasks);
    
    mgr->m_watcher_ctx = NULL;
    mgr->m_watcher_fun = NULL;
    
    if (mgr->m_protocol) {
        net_trans_protocol_free(mgr->m_protocol);
        mgr->m_protocol = NULL;
    }

    if (mgr->m_request_id_tag) {
        mem_free(mgr->m_alloc, mgr->m_request_id_tag);
        mgr->m_request_id_tag = NULL;
    }
    
    while(!TAILQ_EMPTY(&mgr->m_free_tasks)) {
        net_trans_task_real_free(TAILQ_FIRST(&mgr->m_free_tasks));
    }

    mem_free(mgr->m_alloc, mgr);
}

uint8_t net_trans_manage_debug(net_trans_manage_t manage) {
    return manage->m_debug;
}

void net_trans_manage_set_debug(net_trans_manage_t manage, uint8_t debug) {
    manage->m_debug = debug;
}

const char * net_trans_manage_request_id_tag(net_trans_manage_t manage) {
    return manage->m_request_id_tag ? manage->m_request_id_tag : "";
}

int net_trans_manage_set_request_id_tag(net_trans_manage_t manage, const char * tag) {
    char * new_tag = NULL;

    if (tag) {
        new_tag = cpe_str_mem_dup(manage->m_alloc, tag);
        if (new_tag == NULL) {
            CPE_ERROR(manage->m_em, "trans-cli: dup tag %s fail", tag);
            return -1;
        }
    }

    if (manage->m_request_id_tag) {
        mem_free(manage->m_alloc, manage->m_request_id_tag);
    }

    manage->m_request_id_tag = new_tag;
    
    return 0;
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
