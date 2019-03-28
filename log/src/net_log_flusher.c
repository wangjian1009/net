#include <assert.h>
#include "net_log_flusher_i.h"
#include "net_log_category_i.h"
#include "log_queue.h"

static void * net_log_flusher_thread(void * param);

net_log_flusher_t
net_log_flusher_create(net_log_schedule_t schedule, const char * name) {
    net_log_flusher_t flusher = mem_alloc(schedule->m_alloc, sizeof(struct net_log_flusher));
    if (flusher == NULL) {
        CPE_ERROR(schedule->m_em, "log: flusher %s: alloc fail", name);
        return NULL;
    }

    flusher->m_schedule = schedule;
    cpe_str_dup(flusher->m_name, sizeof(flusher->m_name), name);
    flusher->m_queue = log_queue_create(32);
    if (flusher->m_queue == NULL) {
        CPE_ERROR(schedule->m_em, "log: flusher %s: queue create fail", name);
        mem_free(schedule->m_alloc, flusher);
        return NULL;
    }

    //flusher->m_thread == 
    pthread_mutex_init(&flusher->m_mutex, NULL);
    pthread_cond_init(&flusher->m_cond, NULL);

    TAILQ_INIT(&flusher->m_categories);
    TAILQ_INSERT_TAIL(&schedule->m_flushers, flusher, m_next);
    
    return flusher;
}

void net_log_flusher_free(net_log_flusher_t flusher) {
    net_log_schedule_t schedule = flusher->m_schedule;

    assert(schedule->m_state == net_log_schedule_state_init);

    while(!TAILQ_EMPTY(&flusher->m_categories)) {
        net_log_category_t category = TAILQ_FIRST(&flusher->m_categories);
        category->m_flusher = NULL;
        TAILQ_REMOVE(&flusher->m_categories, category, m_next_for_flusher);
    }
    
    pthread_cond_destroy(&flusher->m_cond);
    pthread_mutex_destroy(&flusher->m_mutex);
    
    TAILQ_REMOVE(&schedule->m_flushers, flusher, m_next);
    mem_free(schedule->m_alloc, flusher);
}

net_log_flusher_t net_log_flusher_find(net_log_schedule_t schedule, const char * name) {
    net_log_flusher_t flusher;

    TAILQ_FOREACH(flusher, &schedule->m_flushers, m_next) {
        if (strcmp(flusher->m_name, name) == 0) return flusher;
    }
    
    return NULL;
}

static void * net_log_flusher_thread(void * param) {
    net_log_flusher_t flusher = param;
    net_log_schedule_t schedule = flusher->m_schedule;

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: flusher %s: thread starated", flusher->m_name);
    }

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: flusher %s: thread stoped", flusher->m_name);
    }
    
    return NULL;
}
