#include "assert.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/string_utils.h"
#include "net_schedule.h"
#include "net_address.h"
#include "net_timer.h"
#include "net_trans_manage_i.h"
#include "net_trans_task_i.h"

net_trans_manage_t net_trans_manage_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule, net_driver_t driver,
    const char * name)
{
    if (curl_global_init(0) != CURLE_OK) {
        CPE_ERROR(em, "trans-cli: CURL global init fail");
        return NULL;
    }
    
    net_trans_manage_t manage = mem_alloc(alloc, sizeof(struct net_trans_manage));
    if (manage == NULL) {
        CPE_ERROR(em, "trans-cli: manage alloc fail");
        curl_global_cleanup();
        return NULL;
    }

    manage->m_alloc = alloc;
    manage->m_em = em;
    manage->m_debug = 0;
    manage->m_schedule = schedule;
    manage->m_driver = driver;
    manage->m_watcher_ctx = NULL;
    manage->m_watcher_fun = NULL;
    cpe_str_dup(manage->m_name, sizeof(manage->m_name), name);
    manage->m_cfg_protect_vpn = 0;

	manage->m_multi_handle = curl_multi_init();
    manage->m_timer_event = net_timer_create(driver, net_trans_do_timeout, manage);
    manage->m_still_running = 0;
    
    manage->m_max_task_id = 0;

    TAILQ_INIT(&manage->m_free_tasks);

    curl_multi_setopt(manage->m_multi_handle, CURLMOPT_SOCKETFUNCTION, net_trans_sock_cb);
    curl_multi_setopt(manage->m_multi_handle, CURLMOPT_SOCKETDATA, manage);
    curl_multi_setopt(manage->m_multi_handle, CURLMOPT_TIMERFUNCTION, net_trans_timer_cb);
    curl_multi_setopt(manage->m_multi_handle, CURLMOPT_TIMERDATA, manage);
    
    if (cpe_hash_table_init(
            &manage->m_tasks,
            alloc,
            (cpe_hash_fun_t) net_trans_task_hash,
            (cpe_hash_eq_t) net_trans_task_eq,
            CPE_HASH_OBJ2ENTRY(net_trans_task, m_hh_for_mgr),
            -1) != 0)
    {
        mem_free(alloc, manage);
        curl_global_cleanup();
        return NULL;
    }

    
    return manage;
}

void net_trans_manage_free(net_trans_manage_t mgr) {
    net_trans_task_free_all(mgr);
    cpe_hash_table_fini(&mgr->m_tasks);

    if (mgr->m_multi_handle) {
        curl_multi_cleanup(mgr->m_multi_handle);
        mgr->m_multi_handle = NULL;
    }

    if (mgr->m_timer_event) {
        net_timer_free(mgr->m_timer_event);
        mgr->m_timer_event = NULL;
    }
    
    mgr->m_watcher_ctx = NULL;
    mgr->m_watcher_fun = NULL;
    
    while(!TAILQ_EMPTY(&mgr->m_free_tasks)) {
        net_trans_task_real_free(TAILQ_FIRST(&mgr->m_free_tasks));
    }

    mem_free(mgr->m_alloc, mgr);
    curl_global_cleanup();
}

uint8_t net_trans_manage_debug(net_trans_manage_t manage) {
    return manage->m_debug;
}

void net_trans_manage_set_debug(net_trans_manage_t manage, uint8_t debug) {
    manage->m_debug = debug;
}

const char * net_trans_manage_name(net_trans_manage_t manage) {
    return manage->m_name;
}

void net_trans_manage_set_protect_vpn(net_trans_manage_t manage, uint8_t protect_vpn) {
    manage->m_cfg_protect_vpn = protect_vpn;
}

int net_trans_manage_set_pipelining_stream(net_trans_manage_t manage) {
    if (curl_multi_setopt(manage->m_multi_handle, CURLMOPT_PIPELINING, CURLPIPE_MULTIPLEX) != CURLM_OK) {
        CPE_ERROR(manage->m_em, "trans-cli %s: set pipelining stream fail", manage->m_name);
        return -1;
    }

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
