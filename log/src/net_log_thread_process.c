#include <assert.h>
#include "net_schedule.h"
#include "net_log_thread_i.h"
#include "net_log_thread_cmd.h"
#include "net_log_request.h"
#include "net_log_builder.h"
#include "net_log_category_i.h"
#include "net_log_request_cache.h"
#include "net_log_env_i.h"
#include "net_log_statistic_i.h"
#include "net_log_statistic_monitor_i.h"

static void net_log_thread_process_package_pack(net_log_schedule_t schedule, net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
    struct net_log_thread_cmd_package_pack* pack_cmd = (struct net_log_thread_cmd_package_pack*)cmd;
    assert(pack_cmd->head.m_size = sizeof(*pack_cmd));

    net_log_builder_t builder = pack_cmd->m_builder;
    net_log_category_t category = builder->m_category;

    net_log_request_param_t send_param = net_log_category_build_request(category, builder);
    net_log_group_destroy(builder);

    if (send_param == NULL) {
        CPE_ERROR(
            schedule->m_em, "log: thread %s: category [%d]%s: commit: build request fail", 
            log_thread->m_name, category->m_id, category->m_name);
        net_log_statistic_package_discard(log_thread, category, net_log_discard_reason_pack_fail);
        return;
    }
    
    struct net_log_thread_cmd_package_send cmd_send;
    cmd_send.head.m_size = sizeof(cmd_send);
    cmd_send.head.m_cmd = net_log_thread_cmd_type_package_send;
    cmd_send.m_send_param = send_param;

    if (net_log_thread_send_cmd(category->m_flusher, (net_log_thread_cmd_t)&cmd_send, log_thread) != 0) {
        net_log_statistic_package_discard(log_thread, category, net_log_discard_reason_queue_to_send_fail);
        net_log_request_param_free(send_param);
        return;
    }
}

static void net_log_thread_process_package_send(net_log_schedule_t schedule, net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
    struct net_log_thread_cmd_package_send* send_cmd = (struct net_log_thread_cmd_package_send*)cmd;
    assert(send_cmd->head.m_size = sizeof(*send_cmd));

    net_log_request_param_t send_param = send_cmd->m_send_param;
    assert(send_param);

    net_log_request_cache_t cache = TAILQ_FIRST(&log_thread->m_caches);
    if (cache == NULL) { /*没有使用中的缓存 */
        if (log_thread->m_request_buf_size < log_thread->m_schedule->m_cfg_cache_mem_capacity) { /*没有超过内存限制 */
            net_log_request_t request = net_log_request_create(log_thread, send_param);
            if (request == NULL) {
                CPE_ERROR(schedule->m_em, "log: thread %s: send: create request fail", log_thread->m_name);
                goto SEND_FAIL;
            }

            net_log_thread_check_active_requests(log_thread);
            return;
        }
    }

    if (cache == NULL || cache->m_state != net_log_request_cache_building) {
        cache = net_log_request_cache_create(log_thread, log_thread->m_cache_max_id + 1, net_log_request_cache_building, 1);
        if (cache == NULL) {
            CPE_ERROR(schedule->m_em, "log: thread %s: send: create cache fail", log_thread->m_name);
            goto SEND_FAIL;
        }
        log_thread->m_cache_max_id++;
        net_log_statistic_cache_created(log_thread);
    }

    if (net_log_request_cache_append(cache, send_param) != 0) {
        CPE_ERROR(schedule->m_em, "log: thread %s: send: append to cache %d fail", log_thread->m_name, cache->m_id);
        goto SEND_FAIL;
    }

    if (cache->m_size >= log_thread->m_schedule->m_cfg_cache_file_capacity) {
        net_log_request_cache_close(cache);
    }

    net_log_request_param_free(send_param);
    return;
    
SEND_FAIL:
    net_log_statistic_package_discard(log_thread, send_param->category, net_log_discard_reason_queue_to_send_fail);
    net_log_request_param_free(send_param);
}

static void net_log_thread_process_update_env(net_log_schedule_t schedule, net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
    struct net_log_thread_cmd_update_env* update_env_cmd = (struct net_log_thread_cmd_update_env*)cmd;
    assert(update_env_cmd->head.m_size = sizeof(*update_env_cmd));
    
    if (log_thread->m_env_active == update_env_cmd->m_env) return;

    if (schedule->m_debug) {
        CPE_INFO(
            schedule->m_em, "log: thread %s: update env: %s ==> %s",
            log_thread->m_name, 
            log_thread->m_env_active ? log_thread->m_env_active->m_url : "N/A",
            update_env_cmd->m_env ? update_env_cmd->m_env->m_url : "N/A");
    }

    log_thread->m_env_active = update_env_cmd->m_env;
    
    if (log_thread->m_env_active == NULL) {
        while(!TAILQ_EMPTY(&log_thread->m_active_requests)) {
            net_log_request_cancel(TAILQ_FIRST(&log_thread->m_active_requests));
        }
    }
    else {
        net_log_thread_check_active_requests(log_thread);
    }
}

static void net_log_thread_process_update_net(net_log_schedule_t schedule, net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
    struct net_log_thread_cmd_update_net* update_net_cmd = (struct net_log_thread_cmd_update_net*)cmd;
    assert(update_net_cmd->head.m_size = sizeof(*update_net_cmd));
    
    if (log_thread->m_net_schedule != schedule->m_net_schedule) {
        net_schedule_set_local_ip_stack(log_thread->m_net_schedule, update_net_cmd->m_local_ip_stack);
    }
    
    if (update_net_cmd->m_local_ip_stack == net_local_ip_stack_none) {
        while(!TAILQ_EMPTY(&log_thread->m_active_requests)) {
            net_log_request_cancel(TAILQ_FIRST(&log_thread->m_active_requests));
        }
    }
    else {
        net_log_thread_check_active_requests(log_thread);
    }
}

static void net_log_thread_process_stop_force(net_log_schedule_t schedule, net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
    if (log_thread->m_state == net_log_thread_state_stoped) return;

    if (log_thread->m_processor.m_stop) {
        log_thread->m_processor.m_stop(log_thread->m_processor.m_ctx, log_thread);
    }
    
    net_log_thread_set_state(log_thread, net_log_thread_state_stoped);
}

static void net_log_thread_process_stop_begin(net_log_schedule_t schedule, net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
    if (net_log_thread_request_is_empty(log_thread)) {
        net_log_thread_process_stop_force(schedule, log_thread, NULL);
    }
    else {
        net_log_thread_set_state(log_thread, net_log_thread_state_stoping);
    }
}

static void net_log_thread_process_stoped(net_log_schedule_t schedule, net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
    ASSERT_ON_THREAD_MAIN(schedule);

    struct net_log_thread_cmd_stoped* stoped_cmd = (struct net_log_thread_cmd_stoped*)cmd;
    assert(stoped_cmd->head.m_size = sizeof(*stoped_cmd));
    net_log_thread_wait_stop(stoped_cmd->log_thread);
}

static void net_log_thread_process_statistic_package_success(net_log_schedule_t schedule, net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
    ASSERT_ON_THREAD_MAIN(schedule);

    struct net_log_thread_cmd_staistic_package_success * package_success_cmd = (struct net_log_thread_cmd_staistic_package_success *)cmd;
    net_log_category_t category = package_success_cmd->m_category;
    category->m_statistic.m_package_success_count++;
}

static void net_log_thread_process_statistic_package_discard(net_log_schedule_t schedule, net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
    ASSERT_ON_THREAD_MAIN(schedule);

    struct net_log_thread_cmd_staistic_package_discard * package_discard_cmd = (struct net_log_thread_cmd_staistic_package_discard *)cmd;

    net_log_category_t category = package_discard_cmd->m_category;
    net_log_discard_reason_t reason = package_discard_cmd->m_reason;

    category->m_statistic.m_package_discard_count[reason]++;
    
    net_log_statistic_monitor_t monitor;
    TAILQ_FOREACH(monitor, &schedule->m_statistic_monitors, m_next) {
        if (monitor->m_on_discard) {
            monitor->m_on_discard(monitor->m_monitor_ctx, category, reason);
        }
    }
}

static void net_log_thread_process_statistic_op_error(net_log_schedule_t schedule, net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
    ASSERT_ON_THREAD_MAIN(schedule);

    struct net_log_thread_cmd_staistic_op_error * op_error_cmd = (struct net_log_thread_cmd_staistic_op_error *)cmd;

    net_log_env_t env = op_error_cmd->m_env;
    net_log_category_t category = op_error_cmd->m_category;
    
    net_log_statistic_monitor_t monitor;
    TAILQ_FOREACH(monitor, &schedule->m_statistic_monitors, m_next) {
        if (monitor->m_on_op_error) {
            monitor->m_on_op_error(
                monitor->m_monitor_ctx, env, category,
                op_error_cmd->m_trans_error, op_error_cmd->m_http_code, op_error_cmd->m_http_msg);
        }
    }
}

static void net_log_thread_process_statistic_cache_loaded(net_log_schedule_t schedule, net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
    ASSERT_ON_THREAD_MAIN(schedule);

    struct net_log_thread_cmd_staistic_cache_loaded * cache_loaded_cmd = (struct net_log_thread_cmd_staistic_cache_loaded *)cmd;

    net_log_category_t category = cache_loaded_cmd->m_category;
    category->m_statistic.m_loaded_package_count += cache_loaded_cmd->m_package_count;
    category->m_statistic.m_loaded_record_count += cache_loaded_cmd->m_record_count;
    category->m_statistic.m_package_count += cache_loaded_cmd->m_package_count;
    category->m_statistic.m_record_count += cache_loaded_cmd->m_record_count;
}

static void net_log_thread_process_statistic_cache_created(net_log_schedule_t schedule, net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
    ASSERT_ON_THREAD_MAIN(schedule);
    schedule->m_statistic.m_cache_created++;
}

static void net_log_thread_process_statistic_cache_destoried(net_log_schedule_t schedule, net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
    ASSERT_ON_THREAD_MAIN(schedule);
    schedule->m_statistic.m_cache_destoried++;
}

static void net_log_thread_process_statistic_cache_discard(net_log_schedule_t schedule, net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
    ASSERT_ON_THREAD_MAIN(schedule);
    schedule->m_statistic.m_cache_discard++;
}

static struct {
    net_log_thread_cmd_type_t m_cmd;
    void (*m_fun)(net_log_schedule_t schedule, net_log_thread_t log_thread, net_log_thread_cmd_t cmd);
} s_processors[] = {
    { net_log_thread_cmd_type_package_pack, net_log_thread_process_package_pack }
    , { net_log_thread_cmd_type_package_send, net_log_thread_process_package_send }
    , { net_log_thread_cmd_type_update_env, net_log_thread_process_update_env }
    , { net_log_thread_cmd_type_update_net, net_log_thread_process_update_net }
    , { net_log_thread_cmd_type_stop_begin, net_log_thread_process_stop_begin }
    , { net_log_thread_cmd_type_stop_force, net_log_thread_process_stop_force }
    , { net_log_thread_cmd_type_stoped, net_log_thread_process_stoped }
    , { net_log_thread_cmd_type_staistic_package_discard, net_log_thread_process_statistic_package_discard }
    , { net_log_thread_cmd_type_staistic_package_success, net_log_thread_process_statistic_package_success }
    , { net_log_thread_cmd_type_staistic_op_error, net_log_thread_process_statistic_op_error }
    , { net_log_thread_cmd_type_staistic_cache_loaded, net_log_thread_process_statistic_cache_loaded }
    , { net_log_thread_cmd_type_staistic_cache_created, net_log_thread_process_statistic_cache_created }
    , { net_log_thread_cmd_type_staistic_cache_destoried, net_log_thread_process_statistic_cache_destoried }
    , { net_log_thread_cmd_type_staistic_cache_discard, net_log_thread_process_statistic_cache_discard }
};

void net_log_thread_dispatch(net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD(log_thread);
    
    assert(cmd->m_cmd >= 0 && cmd->m_cmd < CPE_ARRAY_SIZE(s_processors));
    assert(s_processors[cmd->m_cmd].m_cmd == cmd->m_cmd);
    s_processors[cmd->m_cmd].m_fun(schedule, log_thread, cmd);
}
