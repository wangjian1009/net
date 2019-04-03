#include <assert.h>
#include "ev.h"
#include "cpe/pal/pal_errno.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "net_schedule.h"
#include "net_driver.h"
#include "net_ev_driver.h"
#include "net_log_sender_i.h"
#include "net_log_category_i.h"
#include "net_log_request_manage.h"
#include "net_log_request_pipe.h"
#include "net_log_request_cmd.h"
#include "net_log_queue.h"
#include "net_log_state_i.h"

static void * net_log_sender_thread(void * param);
static void net_log_send_stop(void* ctx);

net_log_sender_t
net_log_sender_create(net_log_schedule_t schedule, const char * name) {
    net_log_sender_t sender = mem_alloc(schedule->m_alloc, sizeof(struct net_log_sender));
    if (sender == NULL) {
        CPE_ERROR(schedule->m_em, "log: %s: sender: alloc fail", name);
        return NULL;
    }

    sender->m_schedule = schedule;
    cpe_str_dup(sender->m_name, sizeof(sender->m_name), name);
    sender->m_cfg_active_request_count = 1;
    sender->m_request_pipe = net_log_request_pipe_create(schedule, sender->m_name);
    if (sender->m_request_pipe == NULL) {
        CPE_ERROR(schedule->m_em, "log: %s: sender: queue request pipe fail", name);
        mem_free(schedule->m_alloc, sender);
        return NULL;
    }
    sender->m_thread = NULL;

    TAILQ_INIT(&sender->m_categories);
    TAILQ_INSERT_TAIL(&schedule->m_senders, sender, m_next);
    
    return sender;
}

void net_log_sender_free(net_log_sender_t sender) {
    net_log_schedule_t schedule = sender->m_schedule;

    assert(net_log_schedule_state(schedule) == net_log_schedule_state_init);
    assert(sender->m_thread == NULL);

    while(!TAILQ_EMPTY(&sender->m_categories)) {
        net_log_category_t category = TAILQ_FIRST(&sender->m_categories);
        category->m_sender = NULL;
        TAILQ_REMOVE(&sender->m_categories, category, m_next_for_sender);
    }
    
    TAILQ_REMOVE(&schedule->m_senders, sender, m_next);
    mem_free(schedule->m_alloc, sender);
}

void net_log_sender_set_active_request_count(net_log_sender_t sender, uint8_t active_request_count) {
    sender->m_cfg_active_request_count = active_request_count;
}

net_log_sender_t net_log_sender_find(net_log_schedule_t schedule, const char * name) {
    net_log_sender_t sender;

    TAILQ_FOREACH(sender, &schedule->m_senders, m_next) {
        if (strcmp(sender->m_name, name) == 0) return sender;
    }
    
    return NULL;
}

int net_log_sender_start(net_log_sender_t sender) {
    net_log_schedule_t schedule = sender->m_schedule;

    if (sender->m_thread) {
        CPE_ERROR(schedule->m_em, "log: %s: sender: start: thread already started", sender->m_name);
        return -1;
    }

    sender->m_thread = mem_alloc(schedule->m_alloc, sizeof(pthread_t));
    if (sender->m_thread == NULL) {
        CPE_ERROR(schedule->m_em, "log: %s: sender: start: alloc fail", sender->m_name);
        return -1;
    }

    if (pthread_create(sender->m_thread, NULL, net_log_sender_thread, sender) != 0) {
        CPE_ERROR(
            schedule->m_em, "log: %s: sender: start: create thread fail, error=%d (%s)",
            sender->m_name, errno, strerror(errno));
        mem_free(schedule->m_alloc, sender->m_thread);
        sender->m_thread = NULL;
        return -1;
    }

    schedule->m_runing_thread_count++;
    return 0;
}

void net_log_sender_notify_stop(net_log_sender_t sender) {
    net_log_schedule_t schedule = sender->m_schedule;

    if (sender->m_thread == NULL) {
        CPE_ERROR(schedule->m_em, "log: %s: sender: notify stop: thread already stoped", sender->m_name);
        return;
    }

    struct net_log_request_cmd cmd;
    cmd.m_size = sizeof(cmd);
    cmd.m_cmd = net_log_request_cmd_stop;
    if (net_log_request_pipe_send_cmd(sender->m_request_pipe, &cmd) != 0) {
        CPE_ERROR(schedule->m_em, "log: %s: sender: notify stop: send stop cmd fail", sender->m_name);
        return;
    }
    
    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: %s: sender: notify stop: notify success", sender->m_name);
    }
}

void net_log_sender_wait_stop(net_log_sender_t sender) {
    net_log_schedule_t schedule = sender->m_schedule;

    if (sender->m_thread == NULL) {
        CPE_ERROR(schedule->m_em, "log: %s: sender: wait stop: thread already stoped", sender->m_name);
        return;
    }

    if (pthread_join(*sender->m_thread, NULL) != 0) {
        CPE_ERROR(schedule->m_em, "log: %s: sender: wait stop: wait error, errno=%d (%s)", sender->m_name, errno, strerror(errno));
    }

    mem_free(schedule->m_alloc, sender->m_thread);
    sender->m_thread = NULL;

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: %s: sender: wait stop: wait success", sender->m_name);
    }

    assert(schedule->m_runing_thread_count > 0);
    schedule->m_runing_thread_count--;
    if (schedule->m_runing_thread_count == 0) {
        net_log_state_fsm_apply_evt(schedule, net_log_state_fsm_evt_stop_complete);
    }
}

static void * net_log_sender_thread(void * param) {
    net_log_sender_t sender = param;
    net_log_schedule_t schedule = sender->m_schedule;

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: %s: sender: thread starated", sender->m_name);
    }

    struct mem_buffer tmp_buffer;
    mem_buffer_init(&tmp_buffer, schedule->m_alloc);

    struct ev_loop * ev_loop = NULL;
    net_schedule_t net_schedule = NULL;
    net_ev_driver_t ev_driver = NULL;
    net_log_request_manage_t request_mgr = NULL;

    ev_loop = ev_loop_new(EVFLAG_AUTO);
    if (ev_loop == NULL) {
        CPE_ERROR(schedule->m_em, "log: %s: sender: create ev loop fail", sender->m_name);
        goto THREAD_COMPLETED;
    }

    net_schedule = net_schedule_create(schedule->m_alloc, schedule->m_em, 128/*not use this buf*/);
    if (net_schedule == NULL) {
        CPE_ERROR(schedule->m_em, "log: %s: sender: create local net schedule fail", sender->m_name);
        goto THREAD_COMPLETED;
    }

    ev_driver = net_ev_driver_create(net_schedule, ev_loop);
    if (ev_driver == NULL) {
        CPE_ERROR(schedule->m_em, "log: %s: sender: create local ev driver fail", sender->m_name);
        goto THREAD_COMPLETED;
    }

    request_mgr = net_log_request_manage_create(
        schedule, net_schedule, net_driver_from_data(ev_driver),
        sender->m_cfg_active_request_count, sender->m_name, &tmp_buffer,
        net_log_send_stop, ev_loop);
    if (request_mgr == NULL) {
        CPE_ERROR(schedule->m_em, "log: %s: sender: create request mgr fail", sender->m_name);
        goto THREAD_COMPLETED;
    }
    
    if (net_log_request_pipe_bind(sender->m_request_pipe, request_mgr) != 0) {
        CPE_ERROR(schedule->m_em, "log: %s: sender: bind request mgr fail", sender->m_name);
        goto THREAD_COMPLETED;
    }

    ev_set_userdata(ev_loop, request_mgr);

    if (schedule->m_cfg_cache_dir) { /*加载缓存 */
        if (net_log_request_mgr_init_cache_dir(request_mgr) != 0
            || net_log_request_mgr_search_cache(request_mgr) != 0)
        {
            goto THREAD_COMPLETED;
        }

        net_log_request_mgr_check_active_requests(request_mgr);
    }
    
    ev_run(ev_loop, 0);

THREAD_COMPLETED:
    if (request_mgr) {
        if (schedule->m_cfg_cache_dir) { /*保存缓存 */
            net_log_request_mgr_save_and_clear_requests(request_mgr);
        }

        if (sender->m_request_pipe->m_bind_to == request_mgr) {
            net_log_request_pipe_unbind(sender->m_request_pipe);
        }

        net_log_request_manage_free(request_mgr);
    }

    if (net_schedule) {
        net_schedule_free(net_schedule);
    }

    if (ev_loop) {
        ev_loop_destroy(ev_loop);
    }

    mem_buffer_clear(&tmp_buffer);
    
    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: %s: sender: thread stoped", sender->m_name);
    }
    
    return NULL;
}

static void net_log_send_stop(void* ctx) {
    struct ev_loop * ev_loop = ctx;
    ev_break(ev_loop, EVBREAK_ALL);
}
