#include "cpe/utils/stream_buffer.h"
#include "net_schedule.h"
#include "net_log_thread_cmd.h"
#include "net_log_builder.h"
#include "net_log_category_i.h"
#include "net_log_request.h"
#include "net_log_env_i.h"

void net_log_thread_cmd_print(write_stream_t ws, net_log_thread_cmd_t cmd) {
    switch(cmd->m_cmd) {
    case net_log_thread_cmd_type_package_pack:
        stream_printf(ws, "pack(category=%s)", ((struct net_log_thread_cmd_package_pack *)cmd)->m_builder->m_category->m_name);
        break;
    case net_log_thread_cmd_type_package_send: 
        stream_printf(ws, "send(category=%s)", ((struct net_log_thread_cmd_package_send *)cmd)->m_send_param->category->m_name);
        break;
    case net_log_thread_cmd_type_update_env: {
        net_log_env_t env = ((struct net_log_thread_cmd_update_env *)cmd)->m_env;
        stream_printf(ws, "update-env(url=%s)", env ? env->m_url : "N/A");
        break;
    }
    case net_log_thread_cmd_type_update_net:
        stream_printf(ws, "update-net(local-ip-stack=%s)", net_local_ip_stack_str(((struct net_log_thread_cmd_update_net *)cmd)->m_local_ip_stack));
        break;
    case net_log_thread_cmd_type_stop_begin:
        stream_printf(ws, "stop-begin");
        break;
    case net_log_thread_cmd_type_stop_force:
        stream_printf(ws, "stop-force");
        break;
    case net_log_thread_cmd_type_stoped:
        stream_printf(ws, "stoped");
        break;
    case net_log_thread_cmd_type_staistic_package_discard: {
        struct net_log_thread_cmd_staistic_package_discard * discard_cmd = (struct net_log_thread_cmd_staistic_package_discard *)cmd;
        stream_printf(
            ws, "statistic-package-discard(category=%s,reason=%s,env=%s)",
            discard_cmd->m_category->m_name,
            net_log_discard_reason_str(discard_cmd->m_reason),
            discard_cmd->m_env ? discard_cmd->m_env->m_url : "N/A");
        break;
    }
    case net_log_thread_cmd_type_staistic_package_success: {
        struct net_log_thread_cmd_staistic_package_success * success_cmd = (struct net_log_thread_cmd_staistic_package_success *)cmd;
        stream_printf(
            ws, "statistic-package-success(category=%s,env=%s)",
            success_cmd->m_category->m_name,
            success_cmd->m_env ? success_cmd->m_env->m_url : "N/A");
        break;
    }
    }
}

const char * net_log_thread_cmd_dump(mem_buffer_t buffer, net_log_thread_cmd_t cmd) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buffer);

    mem_buffer_clear_data(buffer);

    net_log_thread_cmd_print((write_stream_t)&stream, cmd);
    stream_putc((write_stream_t)&stream, 0);
    
    return mem_buffer_make_continuous(buffer, 0);
}

