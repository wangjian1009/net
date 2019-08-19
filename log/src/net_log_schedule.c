#include "assert.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_stdio.h"
#include "cpe/utils/md5.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/buffer.h"
#include "cpe/utils/time_utils.h"
#include "cpe/fsm/fsm_def.h"
#include "net_schedule.h"
#include "net_address.h"
#include "net_timer.h"
#include "net_local_ip_stack_monitor.h"
#include "net_log_schedule_i.h"
#include "net_log_state_i.h"
#include "net_log_category_i.h"
#include "net_log_thread_i.h"
#include "net_log_thread_cmd.h"
#include "net_log_state_monitor_i.h"
#include "net_log_env_i.h"

static void net_log_schedule_on_local_ip_stack_changed(void * ctx, net_schedule_t net_schedule);

net_log_schedule_t
net_log_schedule_create(
    mem_allocrator_t alloc, error_monitor_t em, uint8_t debug,
    net_schedule_t net_schedule, net_driver_t net_driver, vfs_mgr_t vfs,
    const char * cfg_project, const char * cfg_access_id, const char * cfg_access_key)
{
    net_log_schedule_t schedule = mem_alloc(alloc, sizeof(struct net_log_schedule));
    if (schedule == NULL) {
        CPE_ERROR(em, "log: schedule alloc fail");
        return NULL;
    }

    bzero(schedule, sizeof(*schedule));

    schedule->m_alloc = alloc;
    schedule->m_em = em;
    schedule->m_debug = debug;
    schedule->m_net_schedule = net_schedule;
    schedule->m_net_driver = net_driver;
    schedule->m_vfs = vfs;
    schedule->m_cfg_source = NULL;
    schedule->m_cfg_timeout_ms = 3000;
    schedule->m_cfg_connect_timeout_s = 10;
    schedule->m_cfg_send_timeout_s = 15;
    schedule->m_cfg_cache_mem_capacity = 0;
    schedule->m_cfg_cache_file_capacity = 0;
    schedule->m_cfg_compress = net_log_compress_lz4;
    schedule->m_cfg_active_request_count = 1;
    schedule->m_cfg_dump_span_ms = 0;
    schedule->m_cfg_cache_dir = NULL;
    schedule->m_dump_timer = NULL;
    schedule->m_cfg_dump_span_ms = 0;
    schedule->m_cfg_stop_wait_ms = 3000;
    schedule->m_category_count = 0;
    schedule->m_runing_thread_count = 0;
    schedule->m_env_active = NULL;
    _MS(schedule->m_main_thread_id = pthread_self());
    schedule->m_thread_main = NULL;
    schedule->m_net_monitor = NULL;

    schedule->m_net_monitor = net_local_ip_stack_monitor_create(
        schedule->m_net_schedule, schedule, NULL, net_log_schedule_on_local_ip_stack_changed);
    if (schedule->m_net_monitor == NULL) {
        CPE_ERROR(schedule->m_em, "log: schedule: create net monitor fail!");
        goto CREATE_ERROR;
    }
    
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
    
    schedule->m_state_fsm_def = net_log_create_fsm_def(schedule);
    if (schedule->m_state_fsm_def == NULL) {
        CPE_ERROR(schedule->m_em, "log: schedule: create state fsm def fail!");
        goto CREATE_ERROR;
    }

    if (fsm_machine_init(&schedule->m_state_fsm, schedule->m_state_fsm_def, "init", schedule, "fsm", schedule->m_debug) != 0) {
        CPE_ERROR(schedule->m_em, "log: schedule: init fsm machine fail!");
        goto CREATE_ERROR;
    }

    schedule->m_thread_main = net_log_thread_create(schedule, "main", NULL);
    if (schedule->m_thread_main == NULL) {
        goto CREATE_ERROR;
    }

    TAILQ_INIT(&schedule->m_state_monitors);
    TAILQ_INIT(&schedule->m_threads);
    TAILQ_INIT(&schedule->m_envs);
    
    return schedule;

CREATE_ERROR:
    if (schedule->m_net_monitor) net_local_ip_stack_monitor_free(schedule->m_net_monitor);
    if (schedule->m_state_fsm_def) fsm_def_machine_free(schedule->m_state_fsm_def);
    if (schedule->m_dump_timer) net_timer_free(schedule->m_dump_timer);
    if (schedule->m_cfg_project) mem_free(alloc, schedule->m_cfg_project);
    if (schedule->m_cfg_access_id) mem_free(alloc, schedule->m_cfg_access_id);
    if (schedule->m_cfg_access_key) mem_free(alloc, schedule->m_cfg_access_key);
    if (schedule->m_thread_main) net_log_thread_free(schedule->m_thread_main);
    
    mem_free(alloc, schedule);
    return NULL;
}

void net_log_schedule_free(net_log_schedule_t schedule) {
    ASSERT_ON_THREAD_MAIN(schedule);
    
    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: schedule: free");
    }

    switch(net_log_schedule_state(schedule)) {
    case net_log_schedule_state_runing:
    case net_log_schedule_state_pause:
        net_log_schedule_stop(schedule);
        break;
    default:
        break;
    }
    
    if (net_log_schedule_state(schedule) == net_log_schedule_state_stoping) {
        net_log_schedule_wait_stop_threads(schedule);
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

    /*state-monitor*/
    while(!TAILQ_EMPTY(&schedule->m_state_monitors)) {
        net_log_state_monitor_free(TAILQ_FIRST(&schedule->m_state_monitors));
    }
    
    /*thread*/
    while(!TAILQ_EMPTY(&schedule->m_threads)) {
        net_log_thread_free(TAILQ_FIRST(&schedule->m_threads));
    }
    schedule->m_thread_main = NULL;
    
    /*statistic*/
    while(!TAILQ_EMPTY(&schedule->m_envs)) {
        net_log_env_free(TAILQ_FIRST(&schedule->m_envs));
    }
    
    /*net*/
    if (schedule->m_dump_timer) {
        net_timer_free(schedule->m_dump_timer);
        schedule->m_dump_timer = NULL;
    }

    if (schedule->m_net_monitor) {
        net_local_ip_stack_monitor_free(schedule->m_net_monitor);
        schedule->m_net_monitor = NULL;
    }
    
    /*state*/
    fsm_machine_fini(&schedule->m_state_fsm);
    fsm_def_machine_free(schedule->m_state_fsm_def);
    schedule->m_state_fsm_def = NULL;

    /*cfg*/
    if (schedule->m_cfg_project) {
        mem_free(schedule->m_alloc, schedule->m_cfg_project);
        schedule->m_cfg_project = NULL;
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

    if (schedule->m_cfg_cache_dir) {
        mem_free(schedule->m_alloc, schedule->m_cfg_cache_dir);
        schedule->m_cfg_cache_dir = NULL;
    }
    
    mem_free(schedule->m_alloc, schedule);
}

uint8_t net_log_schedule_debug(net_log_schedule_t schedule) {
    return schedule->m_debug;
}

void net_log_schedule_set_debug(net_log_schedule_t schedule, uint8_t debug) {
    schedule->m_debug = debug;
}

const char * net_log_schedule_cfg_project(net_log_schedule_t schedule) {
    ASSERT_ON_THREAD_MAIN(schedule);
    return schedule->m_cfg_project;
}

const char * net_log_schedule_cfg_ep(net_log_schedule_t schedule) {
    ASSERT_ON_THREAD_MAIN(schedule);
    return schedule->m_env_active ? schedule->m_env_active->m_url : NULL;
}

int net_log_schedule_set_cfg_ep(net_log_schedule_t schedule, const char * cfg_ep) {
    ASSERT_ON_THREAD_MAIN(schedule);
    
    if (cpe_str_cmp_opt(net_log_schedule_cfg_ep(schedule), cfg_ep) == 0) {
        return 0;
    }

    net_log_env_t new_env = NULL;
    if (cfg_ep) {
        new_env = net_log_env_find(schedule, cfg_ep);
        if (new_env == NULL) {
            new_env = net_log_env_create(schedule, cfg_ep);
            if (new_env == NULL) {
                CPE_ERROR(schedule->m_em, "log: schedule: create env fail!");
                return -1;
            }
        }
    }

    net_log_schedule_set_active_env(schedule, new_env);

    return 0;
}

const char * net_log_schedule_cfg_access_id(net_log_schedule_t schedule) {
    return schedule->m_cfg_access_id;
}

const char * net_log_schedule_cfg_access_key(net_log_schedule_t schedule) {
    return schedule->m_cfg_access_key;
}

int net_log_schedule_set_cache_dir(net_log_schedule_t schedule, const char * dir) {
    assert(net_log_schedule_state(schedule) == net_log_schedule_state_init);
    
    char * new_cache_dir = cpe_str_mem_dup(schedule->m_alloc, dir);
    if (new_cache_dir == NULL) {
        CPE_ERROR(schedule->m_em, "log: schedule: dup cache dir '%s' fail", dir);
        return -1;
    }

    if (schedule->m_cfg_cache_dir) {
        mem_free(schedule->m_alloc, schedule->m_cfg_cache_dir);
    }
    schedule->m_cfg_cache_dir = new_cache_dir;
    
    return 0;
}

void net_log_schedule_set_cache_mem_capacity(net_log_schedule_t schedule, uint32_t capacity) {
    assert(net_log_schedule_state(schedule) == net_log_schedule_state_init);
    schedule->m_cfg_cache_mem_capacity = capacity;
}

void net_log_schedule_set_cache_file_capacity(net_log_schedule_t schedule, uint32_t capacity) {
    assert(net_log_schedule_state(schedule) == net_log_schedule_state_init);
    schedule->m_cfg_cache_file_capacity = capacity;
}

net_log_schedule_state_t net_log_schedule_state(net_log_schedule_t schedule) {
    return (net_log_schedule_state_t)schedule->m_state_fsm.m_curent_state;
}

void net_log_schedule_commit(net_log_schedule_t schedule) {
    ASSERT_ON_THREAD_MAIN(schedule);
    
    uint8_t i;
    for(i = 0; i < schedule->m_category_count; ++i) {
        net_log_category_t category = schedule->m_categories[i];
        if (category == NULL) continue;

        if (category->m_builder) {
            net_timer_cancel(category->m_commit_timer);
            net_log_category_commit(category);
        }
    }
}

int net_log_schedule_start(net_log_schedule_t schedule) {
    ASSERT_ON_THREAD_MAIN(schedule);
    
    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: schedule: start!");
    }
    net_log_state_fsm_apply_evt(schedule, net_log_state_fsm_evt_start);
    return 0;
}

void net_log_schedule_stop(net_log_schedule_t schedule) {
    ASSERT_ON_THREAD_MAIN(schedule);

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "log: schedule: stop!");
    }
    net_log_schedule_commit(schedule);
    net_log_state_fsm_apply_evt(schedule, net_log_state_fsm_evt_stop_begin);
}

void net_log_schedule_set_max_active_request_count(net_log_schedule_t schedule, uint8_t max_active_request_count) {
    assert(net_log_schedule_state(schedule) == net_log_schedule_state_init);
    schedule->m_cfg_active_request_count = max_active_request_count;
}

const char * net_log_schedule_state_str(net_log_schedule_state_t schedule_state) {
    switch(schedule_state) {
    case net_log_schedule_state_init:
        return "init";
    case net_log_schedule_state_runing:
        return "runing";
    case net_log_schedule_state_pause:
        return "pause";
    case net_log_schedule_state_stoping:
        return "stoping";
    case net_log_schedule_state_error:
        return "error";
    }
}

void net_log_begin(net_log_schedule_t schedule, uint8_t log_type) {
    ASSERT_ON_THREAD_MAIN(schedule);

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
    if (value) {
        assert(schedule->m_current_category);
        net_log_category_log_apppend(schedule->m_current_category, name, value);
    }
}

void net_log_append_md5(net_log_schedule_t schedule, const char * name, cpe_md5_value_t value) {
    net_log_append_str(schedule, name, cpe_md5_dump(value, net_log_schedule_tmp_buffer(schedule)));
}

void net_log_append_net_address(net_log_schedule_t schedule, const char * name, net_address_t address) {
    net_log_append_str(schedule, name, net_address_dump(net_log_schedule_tmp_buffer(schedule), address));
}

void net_log_commit(net_log_schedule_t schedule) {
    ASSERT_ON_THREAD_MAIN(schedule);
    assert(schedule->m_current_category);
    net_log_category_log_end(schedule->m_current_category);
    schedule->m_current_category = NULL;
}

net_log_thread_t net_log_schedule_thread_main(net_log_schedule_t schedule) {
    ASSERT_ON_THREAD_MAIN(schedule);
    return schedule->m_thread_main;
}

mem_buffer_t net_log_schedule_tmp_buffer(net_log_schedule_t schedule) {
    ASSERT_ON_THREAD_MAIN(schedule);
    return net_schedule_tmp_buffer(schedule->m_net_schedule);
}

static void net_log_schedule_dump_timer(net_timer_t timer, void * ctx) {
    net_log_schedule_t schedule = ctx;
    ASSERT_ON_THREAD_MAIN(schedule);

    CPE_INFO(schedule->m_em, "log: begin dump, category-count=%d", schedule->m_category_count);
    
    /* uint8_t i; */
    /* for(i = 0; i < schedule->m_category_count; ++i) { */
    /*     net_log_category_t category = schedule->m_categories[i]; */
    /*     if (category == NULL) continue; */

    /*     CPE_INFO( */
    /*         schedule->m_em, "log: category [%d]%s: input-log=%d, input-pckage=%d, input-bytes=%.2fM", */
    /*         category->m_id, category->m_name, */
    /*         category->m_statistics_log_count, category->m_statistics_package_count, */
    /*         ((float)category->m_statistics_input_bps.m_total_bytes / 1024.0f / 1024.0f)); */
    /* } */
    
    net_timer_active(schedule->m_dump_timer, schedule->m_cfg_dump_span_ms);
}

int net_log_schedule_start_dump(net_log_schedule_t schedule, uint32_t dump_span_ms) {
    ASSERT_ON_THREAD_MAIN(schedule);
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

void net_log_schedule_check_stop_complete(net_log_schedule_t schedule) {
    if (schedule->m_runing_thread_count != 0) return;
    net_log_state_fsm_apply_evt(schedule, net_log_state_fsm_evt_stop_complete);
    return;
}

void net_log_schedule_set_active_env(net_log_schedule_t schedule, net_log_env_t new_env) {
    ASSERT_ON_THREAD_MAIN(schedule);
    
    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: schedule: ep %s ==> %s", 
            cpe_str_opt(net_log_schedule_cfg_ep(schedule), "N/A"),
            new_env ? new_env->m_url : "N/A");
    }
    
    schedule->m_env_active = new_env;

    struct net_log_thread_cmd_update_env update_env_cmd;
    update_env_cmd.head.m_size = sizeof(update_env_cmd);
    update_env_cmd.head.m_cmd = net_log_thread_cmd_type_update_env;
    update_env_cmd.m_env = new_env;
    
    net_log_thread_t log_thread;
    TAILQ_FOREACH(log_thread, &schedule->m_threads, m_next) {
        if (log_thread->m_is_runing) {
            net_log_thread_send_cmd(log_thread, (net_log_thread_cmd_t)&update_env_cmd, schedule->m_thread_main);
        }
    }
}

static void net_log_schedule_on_local_ip_stack_changed(void * ctx, net_schedule_t net_schedule) {
    net_log_schedule_t schedule = ctx;
    ASSERT_ON_THREAD_MAIN(schedule);

    struct net_log_thread_cmd_update_net update_net_cmd;
    update_net_cmd.head.m_size = sizeof(update_net_cmd);
    update_net_cmd.head.m_cmd = net_log_thread_cmd_type_update_net;
    update_net_cmd.m_local_ip_stack = net_schedule_local_ip_stack(schedule->m_net_schedule);
    
    net_log_thread_t log_thread;
    TAILQ_FOREACH(log_thread, &schedule->m_threads, m_next) {
        if (log_thread->m_is_runing) {
            net_log_thread_send_cmd(log_thread, (net_log_thread_cmd_t)&update_net_cmd, schedule->m_thread_main);
        }
    }
}
