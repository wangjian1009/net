#include <assert.h>
#include "net_log_pipe.h"
#include "net_log_pipe_cmd.h"
#include "net_log_request_manage.h"
#include "net_log_queue.h"
#include "net_log_request.h"

static net_log_request_param_t net_log_pipe_pop_send_param(net_log_pipe_t pipe);

void net_log_pipe_dispatch(net_log_pipe_t pipe, net_log_pipe_cmd_t cmd) {
    net_log_schedule_t schedule = pipe->m_schedule;
    
    switch (cmd->m_cmd) {
    case net_log_pipe_cmd_send: {
        net_log_request_param_t send_param = net_log_pipe_pop_send_param(pipe);
        assert(send_param);

        if (pipe->m_bind_request_mgr) {
            net_log_request_manage_process_cmd_send(pipe->m_bind_request_mgr, send_param);
        } else {
            CPE_ERROR(schedule->m_em, "log: pipe %s: cmd send: no bind request mgr", pipe->m_name);
            //TODO: loki net_log_category_add_fail_statistics(send_param->category, send_param->log_count);
            net_log_request_param_free(send_param);
        }
        break;
    }
    case net_log_pipe_cmd_pause:
        if (pipe->m_bind_request_mgr) {
            net_log_request_manage_process_cmd_pause(pipe->m_bind_request_mgr);
        } else {
            CPE_ERROR(schedule->m_em, "log: pipe %s: cmd pause: no bind request mgr", pipe->m_name);
        }
        break;
    case net_log_pipe_cmd_resume:
        if (pipe->m_bind_request_mgr) {
            net_log_request_manage_process_cmd_resume(pipe->m_bind_request_mgr);
        } else {
            CPE_ERROR(schedule->m_em, "log: pipe %s: cmd resume: no bind request mgr", pipe->m_name);
        }
        break;
    case net_log_pipe_cmd_stop_begin:
        if (pipe->m_bind_request_mgr) {
            net_log_request_manage_process_cmd_stop_begin(pipe->m_bind_request_mgr);
        } else {
            CPE_ERROR(schedule->m_em, "log: pipe %s: cmd stop-begin: no bind request mgr", pipe->m_name);
        }
        break;
    case net_log_pipe_cmd_stop_complete:
        if (pipe->m_bind_request_mgr) {
            net_log_request_manage_process_cmd_stop_complete(pipe->m_bind_request_mgr);
        } else {
            CPE_ERROR(schedule->m_em, "log: pipe %s: cmd stop-complete: no bind request mgr", pipe->m_name);
        }
        break;
    case net_log_pipe_cmd_stoped:
        if (schedule->m_main_thread_pipe == pipe) {
            struct net_log_pipe_cmd_stoped* stoped_cmd = (struct net_log_pipe_cmd_stoped*)cmd;
            assert(stoped_cmd->head.m_size = sizeof(*stoped_cmd));
            net_log_schedule_process_cmd_stoped(schedule, stoped_cmd->owner);
            return;
        } else {
            CPE_ERROR(schedule->m_em, "log: pipe %s: cmd stoped: not in main thread", pipe->m_name);
        }
        break;
    default:
        CPE_ERROR(schedule->m_em, "log: pipe %s: unknown cmd %d", pipe->m_name, cmd->m_cmd);
        break;
    }
}

static net_log_request_param_t net_log_pipe_pop_send_param(net_log_pipe_t pipe) {
    net_log_request_param_t send_param;

    _MS(pthread_mutex_lock(&pipe->m_mutex));
    send_param = net_log_queue_pop(pipe->m_send_param_queue);
    _MS(pthread_mutex_unlock(&pipe->m_mutex));

    return send_param;
}
