#include "assert.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/utils/md5.h"
#include "cpe/utils/string_utils.h"
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
    schedule->m_cfg_project = cpe_str_mem_dup(alloc, cfg_project);

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
        mem_free(alloc, schedule);
        curl_global_cleanup();
        return NULL;
    }
    
    schedule->m_cfg_access_id = cpe_str_mem_dup(alloc, cfg_access_id);
    schedule->m_cfg_access_key = cpe_str_mem_dup(alloc, cfg_access_key);

    mem_buffer_init(&schedule->m_kv_buffer, alloc);

    TAILQ_INIT(&schedule->m_flushers);
    TAILQ_INIT(&schedule->m_senders);
    
    return schedule;
}

void net_log_schedule_free(net_log_schedule_t schedule) {
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
    
    /*cache*/
    if (schedule->m_keys) {
        mem_free(schedule->m_alloc, schedule->m_keys);
        schedule->m_keys = NULL;
        schedule->m_keys_len = NULL;
        schedule->m_values = NULL;
        schedule->m_values_len = NULL;
    }
    
    mem_buffer_clear(&schedule->m_kv_buffer);

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
            net_log_request_manage_create(schedule, schedule->m_net_schedule, schedule->m_net_driver);
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
        schedule->m_main_thread_request_pipe = net_log_request_pipe_create(schedule);
        if (schedule->m_main_thread_request_pipe == NULL) {
            CPE_ERROR(schedule->m_em, "log: schedule: create main thread request pipe fail");
            return -1;
        }
    }

    if (schedule->m_main_thread_request_mgr == NULL) {
        schedule->m_main_thread_request_mgr =
            net_log_request_manage_create(schedule, schedule->m_net_schedule, schedule->m_net_driver);
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

static void net_log_schedule_do_stop(net_log_schedule_t schedule, uint8_t category_count);

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
        net_log_flusher_start(flusher);
    }

    TAILQ_FOREACH(sender, &schedule->m_senders, m_next) {
        net_log_sender_start(sender);
    }
    
    return 0;
}

void net_log_schedule_stop(net_log_schedule_t schedule) {
    if (schedule->m_state == net_log_schedule_state_init) return;
    net_log_schedule_do_stop(schedule, schedule->m_category_count);
}

static void net_log_schedule_do_stop(net_log_schedule_t schedule, uint8_t category_count) {
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
    schedule->m_kv_count = 0;
    mem_buffer_clear_data(&schedule->m_kv_buffer);
    schedule->m_current_category = schedule->m_categories[log_type];
}

void net_log_append_int32(net_log_schedule_t schedule, const char * name, int32_t value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%d", value);
    net_log_append_str(schedule, name, buf);
}

void net_log_append_uint32(net_log_schedule_t schedule, const char * name, uint32_t value) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", value);
    net_log_append_str(schedule, name, buf);
}

void net_log_append_int64(net_log_schedule_t schedule, const char * name, int64_t value) {
    char buf[32];
    snprintf(buf, sizeof(buf), FMT_INT64_T, value);
    net_log_append_str(schedule, name, buf);
}

void net_log_append_uint64(net_log_schedule_t schedule, const char * name, uint64_t value) {
    char buf[32];
    snprintf(buf, sizeof(buf), FMT_UINT64_T, value);
    net_log_append_str(schedule, name, buf);
}

void net_log_append_str(net_log_schedule_t schedule, const char * name, const char * value) {
    if (schedule->m_kv_count >= schedule->m_kv_capacity) {
        uint32_t new_capacity = schedule->m_kv_capacity < 32 ? 32 : schedule->m_kv_capacity * 2;
        while(schedule->m_kv_count >= new_capacity) {
            if (new_capacity > 1024) {
                CPE_ERROR(schedule->m_em, "chain: log: append value overflow, capacity=%d!", schedule->m_kv_capacity);
                return;
            }
            new_capacity *= 2;
        }

        uint32_t p_capacity = sizeof(char *) * new_capacity;
        uint32_t len_capacity = sizeof(size_t) * new_capacity;
        uint32_t buf_capacity = (p_capacity + len_capacity) * 2;
        char * new_buf = mem_alloc(schedule->m_alloc, buf_capacity);
        if (new_buf == NULL) {
            CPE_ERROR(
                schedule->m_em, "chain: log: append  value: alloc array_buf fail, capacity=%d, buf-capacity=%d!",
                new_capacity, buf_capacity);
            return;
        }
        
        char * * new_keys = (char**)new_buf + 0;
        size_t * new_keys_len = (size_t*)(new_buf + p_capacity);
        char * * new_values = (char**)(new_buf + p_capacity + len_capacity);
        size_t * new_values_len = (size_t*)(new_buf + p_capacity + len_capacity + p_capacity);

        if (schedule->m_keys) {
            memcpy(new_keys, schedule->m_keys, sizeof(char *) * schedule->m_kv_count);
            memcpy(new_keys_len, schedule->m_keys_len, sizeof(size_t) * schedule->m_kv_count);
            memcpy(new_values, schedule->m_values, sizeof(char *) * schedule->m_kv_count);
            memcpy(new_values_len, schedule->m_values_len, sizeof(size_t) * schedule->m_kv_count);

            mem_free(schedule->m_alloc, schedule->m_keys);
        }

        schedule->m_keys = new_keys;
        schedule->m_keys_len = new_keys_len;
        schedule->m_values = new_values;
        schedule->m_values_len = new_values_len;
    }

    int32_t pos = schedule->m_kv_count++;

    schedule->m_keys[pos] = mem_buffer_strdup(&schedule->m_kv_buffer, name);
    schedule->m_keys_len[pos] = strlen(name);

    schedule->m_values[pos] = mem_buffer_strdup(&schedule->m_kv_buffer, value);
    schedule->m_values_len[pos] = strlen(value);
}

void net_log_append_md5(net_log_schedule_t schedule, const char * name, cpe_md5_value_t value) {
    net_log_append_str(schedule, name, cpe_md5_dump(value, net_log_schedule_tmp_buffer(schedule)));
}

void net_log_append_net_address(net_log_schedule_t schedule, const char * name, net_address_t address) {
    net_log_append_str(schedule, name, net_address_dump(net_log_schedule_tmp_buffer(schedule), address));
}

void net_log_commit(net_log_schedule_t schedule) {
    if (schedule->m_current_category == NULL) {
        CPE_ERROR(schedule->m_em, "log: commit: no current producer!");
        return;
    }

    net_log_category_t category = schedule->m_current_category;
    
    if (net_log_category_add_log(
            category,
            schedule->m_kv_count, schedule->m_keys, schedule->m_keys_len, schedule->m_values, schedule->m_values_len) != 0)
    {
        CPE_ERROR(
            schedule->m_em, "log: category [%d]%s: commit fail",
            category->m_id, category->m_name);
    }
}

mem_buffer_t net_log_schedule_tmp_buffer(net_log_schedule_t schedule) {
    return net_schedule_tmp_buffer(schedule->m_net_schedule);
}
