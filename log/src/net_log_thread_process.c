#include <assert.h>
#include "net_log_thread_i.h"
#include "net_log_thread_cmd.h"
#include "net_log_request.h"
#include "net_log_builder.h"
#include "net_log_category_i.h"
#include "net_log_request_cache.h"

static void net_log_thread_process_package_pack(net_log_schedule_t schedule, net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
    struct net_log_thread_cmd_package_pack* pack_cmd = (struct net_log_thread_cmd_package_pack*)cmd;
    assert(pack_cmd->head.m_size = sizeof(*pack_cmd));

    net_log_category_pack_request(pack_cmd->m_builder->m_category, pack_cmd->m_builder);
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
        cache = net_log_request_cache_create(log_thread, log_thread->m_cache_max_id + 1, net_log_request_cache_building);
        if (cache == NULL) {
            CPE_ERROR(schedule->m_em, "log: thread %s: send: create cache fail", log_thread->m_name);
            goto SEND_FAIL;
        }
        log_thread->m_cache_max_id++;
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
    //TODO: Loki net_log_category_add_fail_statistics(send_param->category, send_param->log_count);
    net_log_request_param_free(send_param);
}

static void net_log_thread_process_update_env(net_log_schedule_t schedule, net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
}

static void net_log_thread_process_update_net(net_log_schedule_t schedule, net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
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

static void net_log_thread_process_package_success(net_log_schedule_t schedule, net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
    ASSERT_ON_THREAD_MAIN(schedule);

    struct net_log_thread_cmd_staistic_package_success * package_success_cmd = (struct net_log_thread_cmd_staistic_package_success *)cmd;
    net_log_category_statistic_success(package_success_cmd->m_category);
}

static void net_log_thread_process_package_discard(net_log_schedule_t schedule, net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
    ASSERT_ON_THREAD_MAIN(schedule);

    struct net_log_thread_cmd_staistic_package_discard * package_discard_cmd = (struct net_log_thread_cmd_staistic_package_discard *)cmd;
    net_log_category_statistic_discard(package_discard_cmd->m_category, package_discard_cmd->m_reason);
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
    , { net_log_thread_cmd_type_staistic_package_discard, net_log_thread_process_package_discard }
    , { net_log_thread_cmd_type_staistic_package_success, net_log_thread_process_package_success }
};

void net_log_thread_dispatch(net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD(log_thread);
    
    assert(cmd->m_cmd >= 0 && cmd->m_cmd < CPE_ARRAY_SIZE(s_processors));
    assert(s_processors[cmd->m_cmd].m_cmd == cmd->m_cmd);
    s_processors[cmd->m_cmd].m_fun(schedule, log_thread, cmd);
}
