#include "net_schedule.h"
#include "net_timer.h"
#include "net_ping_mgr_i.h"
#include "net_ping_task_i.h"
#include "net_ping_record_i.h"
#include "net_ping_processor_i.h"

static void net_ping_mgr_do_delay_process(net_timer_t timer, void * input_ctx);

net_ping_mgr_t net_ping_mgr_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule, net_driver_t driver, net_trans_manage_t trans_mgr)
{
    net_ping_mgr_t mgr = mem_alloc(alloc, sizeof(struct net_ping_mgr));
    if (mgr == NULL) {
        CPE_ERROR(em, "ping: mgr: alloc fail!");
        return NULL;
    }

    mgr->m_alloc = alloc;
    mgr->m_em = em;
    mgr->m_schedule = schedule;
    mgr->m_driver = driver;
    mgr->m_trans_mgr = trans_mgr;

    mgr->m_icmp_ping_id_max = 0;
    TAILQ_INIT(&mgr->m_tasks_no_notify);
    TAILQ_INIT(&mgr->m_tasks_to_notify);

    TAILQ_INIT(&mgr->m_free_tasks);
    TAILQ_INIT(&mgr->m_free_records);
    TAILQ_INIT(&mgr->m_free_processors);

    mgr->m_delay_process = net_timer_auto_create(mgr->m_schedule, net_ping_mgr_do_delay_process, mgr);
    if (mgr->m_delay_process == NULL) {
        CPE_ERROR(mgr->m_em, "ping: mgr: create delay process timer fail!");
        mem_free(mgr->m_alloc, mgr);
        return NULL;
    }
    
    return mgr;
}

void net_ping_mgr_free(net_ping_mgr_t mgr) {
    if (mgr->m_delay_process) {
        net_timer_free(mgr->m_delay_process);
        mgr->m_delay_process = NULL;
    }
    
    while(!TAILQ_EMPTY(&mgr->m_tasks_no_notify)) {
        net_ping_task_free(TAILQ_FIRST(&mgr->m_tasks_no_notify));
    }

    while(!TAILQ_EMPTY(&mgr->m_tasks_to_notify)) {
        net_ping_task_free(TAILQ_FIRST(&mgr->m_tasks_to_notify));
    }
    
    while(!TAILQ_EMPTY(&mgr->m_free_tasks)) {
        net_ping_task_real_free(TAILQ_FIRST(&mgr->m_free_tasks));
    }

    while(!TAILQ_EMPTY(&mgr->m_free_records)) {
        net_ping_record_real_free(TAILQ_FIRST(&mgr->m_free_records));
    }

    while(!TAILQ_EMPTY(&mgr->m_free_processors)) {
        net_ping_processor_real_free(TAILQ_FIRST(&mgr->m_free_processors));
    }
    
    mem_free(mgr->m_alloc, mgr);
}

void net_ping_mgr_active_delay_process(net_ping_mgr_t mgr) {
    net_timer_active(mgr->m_delay_process, 0);
}

static void net_ping_mgr_do_delay_process(net_timer_t timer, void * input_ctx) {
    net_ping_mgr_t mgr = input_ctx;
    
    while(!TAILQ_EMPTY(&mgr->m_tasks_to_notify)) {
        net_ping_task_t task = TAILQ_FIRST(&mgr->m_tasks_to_notify);
        net_ping_task_set_to_notify(task, 0);
        net_ping_task_do_notify(task);
    }
}

mem_buffer_t net_ping_mgr_tmp_buffer(net_ping_mgr_t mgr) {
    return net_schedule_tmp_buffer(mgr->m_schedule);
}
