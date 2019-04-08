#include <assert.h>
#include "cpe/pal/pal_errno.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_stdlib.h"
#include "cpe/utils/string_utils.h"
#include "net_log_flusher_i.h"
#include "net_log_category_i.h"
#include "net_log_request.h"
#include "net_log_queue.h"
#include "net_log_builder.h"
#include "net_log_state_i.h"
#include "net_log_pipe.h"
#include "net_log_pipe_cmd.h"

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
    flusher->m_queue = net_log_queue_create(32);
    if (flusher->m_queue == NULL) {
        CPE_ERROR(schedule->m_em, "log: flusher %s: queue create fail", name);
        mem_free(schedule->m_alloc, flusher);
        return NULL;
    }

    flusher->m_thread = NULL;
    flusher->m_is_runing = 0;
    pthread_mutex_init(&flusher->m_mutex, NULL);
    pthread_cond_init(&flusher->m_cond, NULL);

    TAILQ_INIT(&flusher->m_categories);
    TAILQ_INSERT_TAIL(&schedule->m_flushers, flusher, m_next);
    
    return flusher;
}

void net_log_flusher_free(net_log_flusher_t flusher) {
    net_log_schedule_t schedule = flusher->m_schedule;

    assert(net_log_schedule_state(schedule) == net_log_schedule_state_init);
    assert(flusher->m_thread == NULL);

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

int net_log_flusher_queue(net_log_flusher_t flusher, net_log_builder_t builder) {
    net_log_schedule_t schedule = flusher->m_schedule;

    pthread_mutex_lock(&flusher->m_mutex);

    if (net_log_queue_push(flusher->m_queue, builder) != 0) {
        CPE_ERROR(schedule->m_em, "log: flusher %s: push log group fail", flusher->m_name);
        pthread_mutex_unlock(&flusher->m_mutex);
        return -1;
    }
    else {
        pthread_cond_signal(&flusher->m_cond);
    }
    
    pthread_mutex_unlock(&flusher->m_mutex);

    return 0;
}

static void * net_log_flusher_thread(void * param) {
    net_log_flusher_t flusher = param;
    net_log_schedule_t schedule = flusher->m_schedule;

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: flusher %s: thread starated", flusher->m_name);
    }

    pthread_mutex_lock(&flusher->m_mutex);

    while(flusher->m_is_runing) {
        net_log_builder_t builder = net_log_queue_pop(flusher->m_queue);
        if (builder == NULL) {
            pthread_cond_wait(&flusher->m_cond, &flusher->m_mutex);
            continue;
        }

        pthread_mutex_unlock(&flusher->m_mutex);

        net_log_category_t category = builder->m_category;

        net_log_request_param_t send_param = net_log_category_build_request(category, builder);

        net_log_group_destroy(builder);
        
        if (send_param == NULL) {
            CPE_ERROR(schedule->m_em, "log: flusher %s: build param fail", flusher->m_name);
        }
        else {
            if (net_log_category_commit_request(category, send_param, 0) != 0) {
                free(send_param);
            }
        }
        
        pthread_mutex_lock(&flusher->m_mutex);
    }
    
    pthread_mutex_unlock(&flusher->m_mutex);

    struct net_log_pipe_cmd_stoped stop_cmd;
    stop_cmd.head.m_size = sizeof(stop_cmd);
    stop_cmd.head.m_cmd = net_log_pipe_cmd_stoped;
    stop_cmd.owner = flusher;
    assert(schedule->m_main_thread_pipe);
    net_log_pipe_send_cmd(schedule->m_main_thread_pipe, (net_log_pipe_cmd_t)&stop_cmd);
    
    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: flusher %s: thread stoped", flusher->m_name);
    }
    
    return NULL;
}

int net_log_flusher_start(net_log_flusher_t flusher) {
    net_log_schedule_t schedule = flusher->m_schedule;

    if (flusher->m_thread) {
        CPE_ERROR(schedule->m_em, "log: flusher %s: start: thread already started", flusher->m_name);
        return -1;
    }

    flusher->m_thread = mem_alloc(schedule->m_alloc, sizeof(pthread_t));
    if (flusher->m_thread == NULL) {
        CPE_ERROR(schedule->m_em, "log: flusher %s: start: alloc fail", flusher->m_name);
        return -1;
    }

    flusher->m_is_runing = 1;
    if (pthread_create(flusher->m_thread, NULL, net_log_flusher_thread, flusher) != 0) {
        CPE_ERROR(
            schedule->m_em, "log: flusher %s: start: create thread fail, error=%d (%s)",
            flusher->m_name, errno, strerror(errno));
        mem_free(schedule->m_alloc, flusher->m_thread);
        flusher->m_thread = NULL;
        flusher->m_is_runing = 0;
        return -1;
    }
    
    schedule->m_runing_thread_count++;
    return 0;
}

void net_log_flusher_notify_stop(net_log_flusher_t flusher) {
    net_log_schedule_t schedule = flusher->m_schedule;

    if (flusher->m_thread == NULL) {
        CPE_ERROR(schedule->m_em, "log: flusher %s: notify stop: thread already stoped", flusher->m_name);
        return;
    }

    pthread_mutex_lock(&flusher->m_mutex);
    flusher->m_is_runing = 0;
    pthread_cond_signal(&flusher->m_cond);
    pthread_mutex_unlock(&flusher->m_mutex);

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: flusher %s: notify stop: notify success", flusher->m_name);
    }
}

void net_log_flusher_wait_stop(net_log_flusher_t flusher) {
    net_log_schedule_t schedule = flusher->m_schedule;

    if (flusher->m_thread == NULL) {
        CPE_ERROR(schedule->m_em, "log: flusher %s: wait stop: thread already stoped", flusher->m_name);
        return;
    }

    if (pthread_join(*flusher->m_thread, NULL) != 0) {
        CPE_ERROR(schedule->m_em, "log: flusher %s: wait stop: wait error, errno=%d (%s)", flusher->m_name, errno, strerror(errno));
    }

    mem_free(schedule->m_alloc, flusher->m_thread);
    flusher->m_thread = NULL;

    assert(schedule->m_runing_thread_count > 0);
    schedule->m_runing_thread_count--;
    
    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: flusher %s: wait stop: wait success, thread-count=%d",
            flusher->m_name, schedule->m_runing_thread_count);
    }

    net_log_schedule_check_stop_complete(schedule);
}

