#include "assert.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/utils/md5.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/buffer.h"
#include "net_schedule.h"
#include "net_address.h"
#include "net_timer.h"
#include "net_log_schedule_i.h"
#include "net_log_category_i.h"
#include "net_log_flusher_i.h"
#include "net_log_sender_i.h"
#include "net_log_request_manage.h"
#include "net_log_request_pipe.h"

net_log_schedule_t
net_log_schedule_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t net_schedule, net_driver_t net_driver,
    const char * cfg_project, const char * cfg_ep, const char * cfg_access_id, const char * cfg_access_key)
{
    CURLcode ecode;
    if ((ecode = curl_global_init(0)) != CURLE_OK) {
        CPE_ERROR(em, "log: schedule: curl_global_init failure, code:%d %s.\n", ecode, curl_easy_strerror(ecode));
        return NULL;
    }
    
    net_log_schedule_t schedule = mem_alloc(alloc, sizeof(struct net_log_schedule));
    if (schedule == NULL) {
        CPE_ERROR(em, "log: schedule alloc fail");
        curl_global_cleanup();
        return NULL;
    }

    bzero(schedule, sizeof(*schedule));

    schedule->m_alloc = alloc;
    schedule->m_em = em;
    schedule->m_debug = 0;
    schedule->m_net_schedule = net_schedule;
    schedule->m_net_driver = net_driver;
    schedule->m_cfg_source = NULL;
    schedule->m_cfg_timeout_ms = 3000;
    schedule->m_cfg_connect_timeout_s = 10;
    schedule->m_cfg_send_timeout_s = 15;
    schedule->m_cfg_compress = net_log_compress_lz4;
    schedule->m_cfg_active_request_count = 1;
    schedule->m_cfg_dump_span_ms = 0;
    schedule->m_dump_timer = NULL;
    schedule->m_cfg_dump_span_ms = 0;

    schedule->m_cfg_project = cpe_str_mem_dup(alloc, cfg_project);
    if (schedule->m_cfg_project == NULL) {
        CPE_ERROR(schedule->m_em, "log: schedule: dup cfg project fail!");
        goto CREATE_ERROR;
    }

    schedule->m_cfg_access_id = cpe_str_mem_dup(alloc, cfg_access_id);
    if (schedule->m_cfg_access_id == NULL) {
        CPE_ERROR(schedule->m_em, "log: schedule: dup cfg access id fail!");
        goto CREATE_ERROR;
    }

    schedule->m_cfg_access_key = cpe_str_mem_dup(alloc, cfg_access_key);
    if (schedule->m_cfg_access_key == NULL) {
        CPE_ERROR(schedule->m_em, "log: schedule: dup cfg access key fail!");
        goto CREATE_ERROR;
    }
    
    if (cpe_str_start_with(cfg_ep, "http://")) {
        schedule->m_cfg_using_https = 0;
        schedule->m_cfg_ep = cpe_str_mem_dup(alloc, cfg_ep + 7);
    }
    else if (cpe_str_start_with(cfg_ep, "https://")) {
        schedule->m_cfg_using_https = 1;
        schedule->m_cfg_ep = cpe_str_mem_dup(alloc, cfg_ep + 8);
    }
    else {
        CPE_ERROR(em, "log: schedule: endpoint %s format error", cfg_ep);
        goto CREATE_ERROR;
    }
    if (schedule->m_cfg_ep == NULL) {
        CPE_ERROR(schedule->m_em, "log: schedule: dup endpoint fail!");
        goto CREATE_ERROR;
    }
    
    TAILQ_INIT(&schedule->m_flushers);
    TAILQ_INIT(&schedule->m_senders);
    
    return schedule;

CREATE_ERROR:
    if (schedule->m_dump_timer) net_timer_free(schedule->m_dump_timer);
    if (schedule->m_cfg_project) mem_free(alloc, schedule->m_cfg_project);
    if (schedule->m_cfg_access_id) mem_free(alloc, schedule->m_cfg_access_id);
    if (schedule->m_cfg_access_key) mem_free(alloc, schedule->m_cfg_access_key);
    if (schedule->m_cfg_ep) mem_free(alloc, schedule->m_cfg_ep);
    
    mem_free(alloc, schedule);
    curl_global_cleanup();
    return NULL;
}

void net_log_schedule_free(net_log_schedule_t schedule) {
    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: schedule: free");
    }
    
    if (schedule->m_state != net_log_schedule_state_init) {
        net_log_schedule_stop(schedule);
    }

    if (schedule->m_main_thread_request_pipe) {
        net_log_request_pipe_free(schedule->m_main_thread_request_pipe);
        schedule->m_main_thread_request_pipe = NULL;
    }
    
    if (schedule->m_main_thread_request_mgr) {
        net_log_request_manage_free(schedule->m_main_thread_request_mgr);
        schedule->m_main_thread_request_mgr = NULL;
    }
    
    uint8_t i;
    for(i = 0; i < schedule->m_category_count; ++i) {
        if (schedule->m_categories[i]) {
            net_log_category_free(schedule->m_categories[i]);
            assert(schedule->m_categories[i] == NULL);
        }
    }

    if (schedule->m_categories) {
        mem_free(schedule->m_alloc, schedule->m_categories);
        schedule->m_categories = NULL;
        schedule->m_category_count = 0;
    }

    /*flusher*/
    while(!TAILQ_EMPTY(&schedule->m_flushers)) {
        net_log_flusher_free(TAILQ_FIRST(&schedule->m_flushers));
    }

    /*sender*/
    while(!TAILQ_EMPTY(&schedule->m_senders)) {
        net_log_sender_free(TAILQ_FIRST(&schedule->m_senders));
    }

    /**/
    if (schedule->m_dump_timer == NULL) {
        net_timer_free(schedule->m_dump_timer);
        schedule->m_dump_timer = NULL;
    }
    
    /*cfg*/
    if (schedule->m_cfg_project) {
        mem_free(schedule->m_alloc, schedule->m_cfg_project);
        schedule->m_cfg_project = NULL;
    }

    if (schedule->m_cfg_ep) {
        mem_free(schedule->m_alloc, schedule->m_cfg_ep);
        schedule->m_cfg_ep = NULL;
    }
    
    if (schedule->m_cfg_access_id) {
        mem_free(schedule->m_alloc, schedule->m_cfg_access_id);
        schedule->m_cfg_access_id = NULL;
    }

    if (schedule->m_cfg_access_key) {
        mem_free(schedule->m_alloc, schedule->m_cfg_access_key);
        schedule->m_cfg_access_key = NULL;
    }
    
    if (schedule->m_cfg_source) {
        mem_free(schedule->m_alloc, schedule->m_cfg_source);
        schedule->m_cfg_source = NULL;
    }

    if (schedule->m_cfg_net_interface) {
        mem_free(schedule->m_alloc, schedule->m_cfg_net_interface);
        schedule->m_cfg_net_interface = NULL;
    }
    
    if (schedule->m_cfg_remote_address) {
        mem_free(schedule->m_alloc, schedule->m_cfg_remote_address);
        schedule->m_cfg_remote_address = NULL;
    }

    curl_global_cleanup();
    
    mem_free(schedule->m_alloc, schedule);
}

uint8_t net_log_schedule_debug(net_log_schedule_t schedule) {
    return schedule->m_debug;
}

void net_log_schedule_set_debug(net_log_schedule_t schedule, uint8_t debug) {
    schedule->m_debug = debug;
}

net_log_schedule_state_t net_log_schedule_state(net_log_schedule_t log_schedule) {
    return log_schedule->m_state;
}

int net_log_schedule_init_main_thread_mgr(net_log_schedule_t schedule) {
    if (schedule->m_main_thread_request_mgr == NULL) {
        schedule->m_main_thread_request_mgr =
            net_log_request_manage_create(
                schedule, schedule->m_net_schedule, schedule->m_net_driver,
                schedule->m_cfg_active_request_count, "main", net_log_schedule_tmp_buffer(schedule));
        if (schedule->m_main_thread_request_mgr == NULL) {
            CPE_ERROR(schedule->m_em, "log: schedule: create main thread request mgr fail");
            return -1;
        }
    }

    return 0;
}

int net_log_schedule_init_main_thread_pipe(net_log_schedule_t schedule) {
    assert(schedule->m_state == net_log_schedule_state_init);

    if (schedule->m_main_thread_request_pipe == NULL) {
        schedule->m_main_thread_request_pipe = net_log_request_pipe_create(schedule, "main");
        if (schedule->m_main_thread_request_pipe == NULL) {
            CPE_ERROR(schedule->m_em, "log: schedule: create main thread request pipe fail");
            return -1;
        }
    }

    if (schedule->m_main_thread_request_mgr == NULL) {
        schedule->m_main_thread_request_mgr =
            net_log_request_manage_create(
                schedule, schedule->m_net_schedule, schedule->m_net_driver,
                schedule->m_cfg_active_request_count, "main", net_log_schedule_tmp_buffer(schedule));
        if (schedule->m_main_thread_request_mgr == NULL) {
            CPE_ERROR(schedule->m_em, "log: schedule: create main thread request mgr fail");
            return -1;
        }
    }

    if (schedule->m_main_thread_request_pipe->m_bind_to == NULL) {
        if (net_log_request_pipe_bind(schedule->m_main_thread_request_pipe, schedule->m_main_thread_request_mgr) != 0) {
            CPE_ERROR(schedule->m_em, "log: schedule: create main thread request pipe bind fail");
            return -1;
        }
    }
    
    return 0;
}

void net_log_schedule_commit(net_log_schedule_t schedule) {
    uint8_t i;
    for(i = 0; i < schedule->m_category_count; ++i) {
        net_log_category_t category = schedule->m_categories[i];
        if (category == NULL) continue;

        if (category->m_builder) {
            net_log_category_commit(category);
        }
    }
}

static void net_log_schedule_do_stop(net_log_schedule_t schedule);

int net_log_schedule_start(net_log_schedule_t schedule) {
    if (schedule->m_state != net_log_schedule_state_init) {
        CPE_ERROR(schedule->m_em, "log: schedule: state is %s, can`t start", net_log_schedule_state_str(schedule->m_state));
        return -1;
    }

    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: schedule: %s ==> %s",
            net_log_schedule_state_str(schedule->m_state), net_log_schedule_state_str(net_log_schedule_state_runing));
    }
    schedule->m_state = net_log_schedule_state_runing;

    net_log_flusher_t flusher;
    net_log_sender_t sender;

    TAILQ_FOREACH(flusher, &schedule->m_flushers, m_next) {
        if (net_log_flusher_start(flusher) != 0) {
            net_log_schedule_do_stop(schedule);
            return -1;
        }
    }

    TAILQ_FOREACH(sender, &schedule->m_senders, m_next) {
        if (net_log_sender_start(sender) != 0) {
            net_log_schedule_do_stop(schedule);
            return -1;
        }
    }
    
    return 0;
}

void net_log_schedule_stop(net_log_schedule_t schedule) {
    if (schedule->m_state == net_log_schedule_state_init) return;

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: schedule: stop begin");
    }
    
    net_log_schedule_do_stop(schedule);

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: schedule: stop complete");
    }
}

void net_log_schedule_set_max_active_request_count(net_log_schedule_t schedule, uint8_t max_active_request_count) {
    assert(schedule->m_state == net_log_schedule_state_init);
    schedule->m_cfg_active_request_count = max_active_request_count;
}

static void net_log_schedule_do_stop(net_log_schedule_t schedule) {
    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: schedule: %s ==> %s",
            net_log_schedule_state_str(schedule->m_state), net_log_schedule_state_str(net_log_schedule_state_stoping));
    }
    schedule->m_state = net_log_schedule_state_stoping;

    net_log_flusher_t flusher;
    net_log_sender_t sender;

    /*首先通知所有线程停止 */
    TAILQ_FOREACH(flusher, &schedule->m_flushers, m_next) {
        if (flusher->m_thread) net_log_flusher_notify_stop(flusher);
    }

    TAILQ_FOREACH(sender, &schedule->m_senders, m_next) {
        if (sender->m_thread) net_log_sender_notify_stop(sender);
    }

    /*然后等待所有线程停止 */
    TAILQ_FOREACH(flusher, &schedule->m_flushers, m_next) {
        if (flusher->m_thread) net_log_flusher_wait_stop(flusher);
    }

    TAILQ_FOREACH(sender, &schedule->m_senders, m_next) {
        if (sender->m_thread) net_log_sender_wait_stop(sender);
    }
    
    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: schedule: %s ==> %s",
            net_log_schedule_state_str(net_log_schedule_state_stoping), net_log_schedule_state_str(net_log_schedule_state_init));
    }
    schedule->m_state = net_log_schedule_state_init;
}

const char * net_log_schedule_state_str(net_log_schedule_state_t schedule_state) {
    switch(schedule_state) {
    case net_log_schedule_state_init:
        return "init";
    case net_log_schedule_state_runing:
        return "runing";
    case net_log_schedule_state_stoping:
        return "stoping";
    }
}

void net_log_begin(net_log_schedule_t schedule, uint8_t log_type) {
    assert(schedule->m_current_category == NULL);
    assert(log_type < schedule->m_category_count);

    net_log_category_t category = schedule->m_categories[log_type];
    schedule->m_current_category = category;
    net_log_category_log_begin(category);
}

void net_log_append_int32(net_log_schedule_t schedule, const char * name, int32_t value) {
    assert(schedule->m_current_category);
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", value);
    net_log_category_log_apppend(schedule->m_current_category, name, buf);
}

void net_log_append_uint32(net_log_schedule_t schedule, const char * name, uint32_t value) {
    assert(schedule->m_current_category);
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", value);
    net_log_category_log_apppend(schedule->m_current_category, name, buf);
}

void net_log_append_int64(net_log_schedule_t schedule, const char * name, int64_t value) {
    assert(schedule->m_current_category);
    char buf[32];
    snprintf(buf, sizeof(buf), FMT_INT64_T, value);
    net_log_category_log_apppend(schedule->m_current_category, name, buf);
}

void net_log_append_uint64(net_log_schedule_t schedule, const char * name, uint64_t value) {
    assert(schedule->m_current_category);
    char buf[32];
    snprintf(buf, sizeof(buf), FMT_UINT64_T, value);
    net_log_category_log_apppend(schedule->m_current_category, name, buf);
}

void net_log_append_str(net_log_schedule_t schedule, const char * name, const char * value) {
    assert(schedule->m_current_category);
    net_log_category_log_apppend(schedule->m_current_category, name, value);
}

void net_log_append_md5(net_log_schedule_t schedule, const char * name, cpe_md5_value_t value) {
    net_log_append_str(schedule, name, cpe_md5_dump(value, net_log_schedule_tmp_buffer(schedule)));
}

void net_log_append_net_address(net_log_schedule_t schedule, const char * name, net_address_t address) {
    net_log_append_str(schedule, name, net_address_dump(net_log_schedule_tmp_buffer(schedule), address));
}

void net_log_commit(net_log_schedule_t schedule) {
    assert(schedule->m_current_category);
    net_log_category_log_end(schedule->m_current_category);
    schedule->m_current_category = NULL;
}

mem_buffer_t net_log_schedule_tmp_buffer(net_log_schedule_t schedule) {
    return net_schedule_tmp_buffer(schedule->m_net_schedule);
}

static void net_log_schedule_dump_timer(net_timer_t timer, void * ctx) {
    net_log_schedule_t schedule = ctx;

    uint8_t i;
    for(i = 0; i < schedule->m_category_count; ++i) {
        net_log_category_t category = schedule->m_categories[i];
        if (category == NULL) continue;

        CPE_INFO(
            schedule->m_em, "log: category [%d]%s: input-log=%d, input-pckage=%d, input-bytes=%.2fM",
            category->m_id, category->m_name,
            category->m_statistics_log_count, category->m_statistics_package_count,
            ((float)category->m_statistics_input_bps.m_total_bytes / 1024.0f / 1024.0f));
    }
    
    net_timer_active(schedule->m_dump_timer, schedule->m_cfg_dump_span_ms);
}

int net_log_schedule_start_dump(net_log_schedule_t schedule, uint32_t dump_span_ms) {
    if (dump_span_ms == 0) {
        if (schedule->m_dump_timer) {
            net_timer_cancel(schedule->m_dump_timer);
        }
    }
    else {
        if (schedule->m_dump_timer == NULL) {
            schedule->m_dump_timer = net_timer_auto_create(schedule->m_net_schedule, net_log_schedule_dump_timer, schedule);
            if (schedule->m_dump_timer == NULL) {
                CPE_ERROR(schedule->m_em, "log: schedule: start dump: create timer fail!");
                return -1;
            }
        }

        schedule->m_cfg_dump_span_ms = dump_span_ms;
        net_timer_active(schedule->m_dump_timer, schedule->m_cfg_dump_span_ms);
    }

    return 0;
}
