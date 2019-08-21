#include <assert.h>
#include "net_log_statistic_i.h"
#include "net_log_thread_i.h"
#include "net_log_thread_cmd.h"
#include "net_log_category_i.h"

void net_log_statistic_package_success(net_log_thread_t log_thread, net_log_category_t category) {
    net_log_schedule_t schedule = category->m_schedule;
    ASSERT_ON_THREAD(log_thread);

    struct net_log_thread_cmd_staistic_package_success package_success_cmd;
    package_success_cmd.head.m_size = sizeof(package_success_cmd);
    package_success_cmd.head.m_cmd = net_log_thread_cmd_type_staistic_package_success;
    package_success_cmd.m_category = category;
    assert(schedule->m_thread_main);
    net_log_thread_send_cmd(schedule->m_thread_main, (net_log_thread_cmd_t)&package_success_cmd, log_thread);
}

void net_log_statistic_package_discard(net_log_thread_t log_thread, net_log_category_t category, net_log_discard_reason_t reason) {
    net_log_schedule_t schedule = category->m_schedule;
    ASSERT_ON_THREAD(log_thread);

    struct net_log_thread_cmd_staistic_package_discard package_discard_cmd;
    package_discard_cmd.head.m_size = sizeof(package_discard_cmd);
    package_discard_cmd.head.m_cmd = net_log_thread_cmd_type_staistic_package_discard;
    package_discard_cmd.m_category = category;
    package_discard_cmd.m_reason = reason;
    net_log_thread_send_cmd(schedule->m_thread_main, (net_log_thread_cmd_t)&package_discard_cmd, log_thread);
}

void net_log_statistic_cache_loaded(net_log_thread_t log_thread, net_log_category_t category, uint32_t package_count, uint32_t record_count) {
    net_log_schedule_t schedule = category->m_schedule;
    ASSERT_ON_THREAD(log_thread);

    struct net_log_thread_cmd_staistic_cache_loaded package_discard_cmd;
    package_discard_cmd.head.m_size = sizeof(package_discard_cmd);
    package_discard_cmd.head.m_cmd = net_log_thread_cmd_type_staistic_cache_loaded;
    /* package_discard_cmd.m_category = category; */
    /* package_discard_cmd.m_found_package_count = package_count; */
    /* package_discard_cmd.m_found_record_count = record_count; */
    net_log_thread_send_cmd(schedule->m_thread_main, (net_log_thread_cmd_t)&package_discard_cmd, log_thread);
}

void net_log_statistic_cache_created(net_log_thread_t log_thread) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD(log_thread);

    struct net_log_thread_cmd_staistic_cache_created package_discard_cmd;
    package_discard_cmd.head.m_size = sizeof(package_discard_cmd);
    package_discard_cmd.head.m_cmd = net_log_thread_cmd_type_staistic_cache_created;
    net_log_thread_send_cmd(schedule->m_thread_main, (net_log_thread_cmd_t)&package_discard_cmd, log_thread);
}

void net_log_statistic_cache_destoried(net_log_thread_t log_thread) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD(log_thread);

    struct net_log_thread_cmd_staistic_cache_destoried package_discard_cmd;
    package_discard_cmd.head.m_size = sizeof(package_discard_cmd);
    package_discard_cmd.head.m_cmd = net_log_thread_cmd_type_staistic_cache_destoried;
    net_log_thread_send_cmd(schedule->m_thread_main, (net_log_thread_cmd_t)&package_discard_cmd, log_thread);
}

void net_log_statistic_cache_discard(net_log_thread_t log_thread) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD(log_thread);

    struct net_log_thread_cmd_staistic_cache_discard package_discard_cmd;
    package_discard_cmd.head.m_size = sizeof(package_discard_cmd);
    package_discard_cmd.head.m_cmd = net_log_thread_cmd_type_staistic_cache_discard;
    net_log_thread_send_cmd(schedule->m_thread_main, (net_log_thread_cmd_t)&package_discard_cmd, log_thread);
}
