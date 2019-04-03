#include "cpe/fsm/fsm_def.h"
#include "net_log_state_i.h"
#include "net_log_flusher_i.h"
#include "net_log_sender_i.h"
#include "net_log_pipe_cmd.h"
#include "net_log_pipe.h"
#include "net_log_request_manage.h"

static void net_log_state_fsm_dump_event(write_stream_t s, fsm_def_machine_t m, void * input_event);

fsm_def_machine_t net_log_create_fsm_def(net_log_schedule_t schedule) {
    fsm_def_machine_t fsm_def = fsm_def_machine_create("log", schedule->m_alloc, schedule->m_em);
    if (fsm_def == NULL) {
        CPE_ERROR(schedule->m_em, "sfox: client: create state fsm def: def create fail!");
        return NULL;
    }

    fsm_def_machine_set_evt_dumper(fsm_def, net_log_state_fsm_dump_event);

    if (net_log_state_fsm_create_init(fsm_def, schedule->m_em) != 0
        || net_log_state_fsm_create_runing(fsm_def, schedule->m_em) != 0
        || net_log_state_fsm_create_pause(fsm_def, schedule->m_em) != 0
        || net_log_state_fsm_create_stoping(fsm_def, schedule->m_em) != 0
        || net_log_state_fsm_create_error(fsm_def, schedule->m_em) != 0
        )
    {
        CPE_ERROR(schedule->m_em, "log: create state fsm def: init fsm fail!");
        fsm_def_machine_free(fsm_def);
        return NULL;
    }

    return fsm_def;
}

int net_log_state_fsm_apply_evt(net_log_schedule_t schedule, net_log_state_fsm_evt_type_t type) {
    struct net_log_state_fsm_evt evt;
    evt.m_type = type;
    return fsm_machine_apply_event(&schedule->m_state_fsm, &evt);
}

static void net_log_state_fsm_dump_event(write_stream_t s, fsm_def_machine_t m, void * input_event) {
    net_log_state_fsm_evt_t evt = input_event;

    switch(evt->m_type) {
    case net_log_state_fsm_evt_start:
        stream_printf(s, "start");
        break;
    case net_log_state_fsm_evt_stop_begin:
        stream_printf(s, "stop-begin");
        break;
    case net_log_state_fsm_evt_stop_complete:
        stream_printf(s, "stop-complete");
        break;
    case net_log_state_fsm_evt_pause:
        stream_printf(s, "pause");
        break;
    case net_log_state_fsm_evt_resume:
        stream_printf(s, "resume");
        break;
    }
}

int net_log_schedule_start_threads(net_log_schedule_t schedule) {
    net_log_flusher_t flusher;
    net_log_sender_t sender;

    TAILQ_FOREACH(flusher, &schedule->m_flushers, m_next) {
        if (net_log_flusher_start(flusher) != 0) {
            if (net_log_schedule_notify_stop_threads(schedule)) {
                net_log_schedule_wait_stop_threads(schedule);
            }
            return -1;
        }
    }

    TAILQ_FOREACH(sender, &schedule->m_senders, m_next) {
        if (net_log_sender_start(sender) != 0) {
            if (net_log_schedule_notify_stop_threads(schedule)) {
                net_log_schedule_wait_stop_threads(schedule);
            }
            return -1;
        }
    }

    return 0;
}

uint8_t net_log_schedule_notify_stop_threads(net_log_schedule_t schedule) {
    net_log_flusher_t flusher;
    net_log_sender_t sender;

    uint8_t have_runing_thread = 0;

    /*首先通知所有线程停止 */
    TAILQ_FOREACH(flusher, &schedule->m_flushers, m_next) {
        if (flusher->m_thread) {
            net_log_flusher_notify_stop(flusher);
            have_runing_thread = 1;
        }
    }

    TAILQ_FOREACH(sender, &schedule->m_senders, m_next) {
        if (sender->m_thread) {
            net_log_sender_notify_stop(sender);
            have_runing_thread = 1;
        }
    }

    return have_runing_thread;
}

void net_log_schedule_wait_stop_threads(net_log_schedule_t schedule) {
    net_log_flusher_t flusher;
    net_log_sender_t sender;

    TAILQ_FOREACH(flusher, &schedule->m_flushers, m_next) {
        if (flusher->m_thread) net_log_flusher_wait_stop(flusher);
    }

    TAILQ_FOREACH(sender, &schedule->m_senders, m_next) {
        if (sender->m_thread) net_log_sender_wait_stop(sender);
    }
}

void net_log_schedule_pause_senders(net_log_schedule_t schedule) {
    struct net_log_pipe_cmd cmd;
    cmd.m_size = sizeof(cmd);
    cmd.m_cmd = net_log_pipe_cmd_pause;

    if (schedule->m_main_thread_request_mgr) {
        net_log_request_manage_process_cmd_pause(schedule->m_main_thread_request_mgr);
    }

    net_log_sender_t sender;
    TAILQ_FOREACH(sender, &schedule->m_senders, m_next) {
        net_log_pipe_send_cmd(sender->m_pipe, &cmd);
    }
}

void net_log_schedule_resume_senders(net_log_schedule_t schedule) {
    struct net_log_pipe_cmd cmd;
    cmd.m_size = sizeof(cmd);
    cmd.m_cmd = net_log_pipe_cmd_resume;

    if (schedule->m_main_thread_request_mgr) {
        net_log_request_manage_process_cmd_resume(schedule->m_main_thread_request_mgr);
    }

    net_log_sender_t sender;
    TAILQ_FOREACH(sender, &schedule->m_senders, m_next) {
        net_log_pipe_send_cmd(sender->m_pipe, &cmd);
    }
}
