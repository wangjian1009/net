#include <assert.h>
#include "net_log_thread_i.h"
#include "net_log_thread_cmd.h"
#include "net_log_request.h"
#include "net_log_builder.h"
#include "net_log_category_i.h"

void net_log_thread_dispatch(net_log_thread_t log_thread, net_log_thread_cmd_t cmd) {
    net_log_schedule_t schedule = log_thread->m_schedule;
    ASSERT_ON_THREAD(log_thread);
    
    switch (cmd->m_cmd) {
    case net_log_thread_cmd_package_pack: {
        struct net_log_thread_cmd_package_pack* pack_cmd = (struct net_log_thread_cmd_package_pack*)cmd;
        assert(pack_cmd->head.m_size = sizeof(*pack_cmd));

        net_log_category_pack_request(pack_cmd->m_builder->m_category, pack_cmd->m_builder);
        break;
    }
    case net_log_thread_cmd_package_send: {
        struct net_log_thread_cmd_package_send* send_cmd = (struct net_log_thread_cmd_package_send*)cmd;
        assert(send_cmd->head.m_size = sizeof(*send_cmd));
        net_log_thread_process_cmd_send(log_thread, send_cmd->m_send_param);
        break;
    }
    case net_log_thread_cmd_pause:
        net_log_thread_process_cmd_pause(log_thread);
        break;
    case net_log_thread_cmd_resume:
        net_log_thread_process_cmd_resume(log_thread);
        break;
    case net_log_thread_cmd_stop_begin:
        net_log_thread_process_cmd_stop_begin(log_thread);
        break;
    case net_log_thread_cmd_stop_force:
        net_log_thread_process_cmd_stop_force(log_thread);
        break;
    case net_log_thread_cmd_stoped:
        ASSERT_ON_THREAD_MAIN(schedule);
        struct net_log_thread_cmd_stoped* stoped_cmd = (struct net_log_thread_cmd_stoped*)cmd;
        assert(stoped_cmd->head.m_size = sizeof(*stoped_cmd));
        net_log_thread_wait_stop(stoped_cmd->log_thread);
        break;
    case net_log_thread_cmd_staistic_package_success: {
        ASSERT_ON_THREAD_MAIN(schedule);
        struct net_log_thread_cmd_staistic_package_success * package_success_cmd = (struct net_log_thread_cmd_staistic_package_success *)cmd;
        net_log_category_statistic_success(package_success_cmd->m_category);
        break;
    }
    case net_log_thread_cmd_staistic_package_discard: {
        ASSERT_ON_THREAD_MAIN(schedule);
        struct net_log_thread_cmd_staistic_package_discard * package_discard_cmd = (struct net_log_thread_cmd_staistic_package_discard *)cmd;
        net_log_category_statistic_discard(package_discard_cmd->m_category, package_discard_cmd->m_reason);
        break;
    }
    default:
        CPE_ERROR(schedule->m_em, "log: thread %s: unknown cmd %d", log_thread->m_name, cmd->m_cmd);
        break;
    }
}
