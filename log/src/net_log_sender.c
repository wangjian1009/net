#include <assert.h>
#include "ev.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_ev_driver.h"
#include "net_log_sender_i.h"
#include "net_log_category_i.h"
#include "net_log_task_manage.h"
#include "log_queue.h"

static void * net_log_sender_thread(void * param);

net_log_sender_t
net_log_sender_create(net_log_schedule_t schedule, const char * name, int32_t queue_size) {
    net_log_sender_t sender = mem_alloc(schedule->m_alloc, sizeof(struct net_log_sender));
    if (sender == NULL) {
        CPE_ERROR(schedule->m_em, "log: sender %s: alloc fail", name);
        return NULL;
    }

    sender->m_schedule = schedule;
    cpe_str_dup(sender->m_name, sizeof(sender->m_name), name);
    sender->m_queue = log_queue_create(queue_size);
    if (sender->m_queue == NULL) {
        CPE_ERROR(schedule->m_em, "log: sender %s: queue create fail", name);
        mem_free(schedule->m_alloc, sender);
        return NULL;
    }

    //sender->m_thread = NULL;
    pthread_mutex_init(&sender->m_mutex, NULL);
    pthread_cond_init(&sender->m_cond, NULL);

    TAILQ_INIT(&sender->m_categories);
    TAILQ_INSERT_TAIL(&schedule->m_senders, sender, m_next);
    
    return sender;
}

void net_log_sender_free(net_log_sender_t sender) {
    net_log_schedule_t schedule = sender->m_schedule;

    assert(schedule->m_state == net_log_schedule_state_init);

    while(!TAILQ_EMPTY(&sender->m_categories)) {
        net_log_category_t category = TAILQ_FIRST(&sender->m_categories);
        category->m_sender = NULL;
        TAILQ_REMOVE(&sender->m_categories, category, m_next_for_sender);
    }
    
    pthread_cond_destroy(&sender->m_cond);
    pthread_mutex_destroy(&sender->m_mutex);
    
    TAILQ_REMOVE(&schedule->m_senders, sender, m_next);
    mem_free(schedule->m_alloc, sender);
}

net_log_sender_t net_log_sender_find(net_log_schedule_t schedule, const char * name) {
    net_log_sender_t sender;

    TAILQ_FOREACH(sender, &schedule->m_senders, m_next) {
        if (strcmp(sender->m_name, name) == 0) return sender;
    }
    
    return NULL;
}

int net_log_sender_queue(net_log_sender_t sender, log_producer_send_param_t send_param) {
    net_log_schedule_t schedule = sender->m_schedule;

    pthread_mutex_lock(&sender->m_mutex);

    if (log_queue_push(sender->m_queue, send_param) != 0) {
        CPE_ERROR(schedule->m_em, "log: sender %s: push param fail", sender->m_name);
        return -1;
    }
    else {
        pthread_cond_signal(&sender->m_cond);
    }
    
    pthread_mutex_unlock(&sender->m_mutex);

    return 0;
}

static void * net_log_sender_thread(void * param) {
    net_log_sender_t sender = param;
    net_log_schedule_t schedule = sender->m_schedule;

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: sender %s: thread starated", sender->m_name);
    }

    struct ev_loop * ev_loop = ev_loop_new(EVFLAG_AUTO);
    if (ev_loop == NULL) {
        CPE_ERROR(schedule->m_em, "log: sender %s: create ev loop fail", sender->m_name);
        return NULL;
    }

    net_schedule_t net_schedule = net_schedule_create(schedule->m_alloc, schedule->m_em, 128/*not use this buf*/);
    if (net_schedule == NULL) {
        CPE_ERROR(schedule->m_em, "log: sender %s: create local net schedule fail", sender->m_name);
        ev_loop_destroy(ev_loop);
        return NULL;
    }

    net_ev_driver_t ev_driver = net_ev_driver_create(net_schedule, ev_loop);
    if (ev_driver == NULL) {
        CPE_ERROR(schedule->m_em, "log: sender %s: create local ev driver fail", sender->m_name);
        net_schedule_free(net_schedule);
        ev_loop_destroy(ev_loop);
        return NULL;
    }

    net_log_task_manage_t task_mgr = net_log_task_manage_create(schedule, net_schedule, net_driver_from_data(ev_driver));
    if (task_mgr == NULL) {
        CPE_ERROR(schedule->m_em, "log: sender %s: create task mgr fail", sender->m_name);
        net_schedule_free(net_schedule);
        ev_loop_destroy(ev_loop);
        return NULL;
    }
    
    ev_set_userdata(ev_loop, task_mgr);
        
    ev_run(ev_loop, 0);

    net_schedule_free(net_schedule);
    ev_loop_destroy(ev_loop);
    
    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: sender %s: thread stoped", sender->m_name);
    }
    
    return NULL;
}

