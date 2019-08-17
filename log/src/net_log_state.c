#include "cpe/fsm/fsm_def.h"
#include "net_log_state_i.h"
#include "net_log_thread_i.h"
#include "net_log_thread_cmd.h"
#include "net_log_request_manage.h"
#include "net_log_category_i.h"
#include "net_log_state_monitor_i.h"

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
    net_log_thread_t thread;

    TAILQ_FOREACH(thread, &schedule->m_threads, m_next) {
        if (net_log_thread_start(thread) != 0) {
            if (net_log_schedule_notify_stop_threads(schedule)) {
                net_log_schedule_wait_stop_threads(schedule);
            }
            return -1;
        }
    }

    return 0;
}

uint8_t net_log_schedule_notify_stop_threads(net_log_schedule_t schedule) {
    net_log_thread_t thread;

    uint8_t have_runing_thread = 0;

    TAILQ_FOREACH(thread, &schedule->m_threads, m_next) {
        if (thread->m_is_runing) {
            net_log_thread_notify_stop_force(thread);
            have_runing_thread = 1;
        }
    }

    return have_runing_thread;
}

void net_log_schedule_wait_stop_threads(net_log_schedule_t schedule) {
    net_log_thread_t thread;

    TAILQ_FOREACH(thread, &schedule->m_threads, m_next) {
        if (thread->m_is_runing) net_log_thread_wait_stop(thread);
    }
}

void net_log_schedule_pause_senders(net_log_schedule_t schedule) {
    struct net_log_thread_cmd cmd;
    cmd.m_size = sizeof(cmd);
    cmd.m_cmd = net_log_thread_cmd_pause;

    net_log_thread_t thread;
    TAILQ_FOREACH(thread, &schedule->m_threads, m_next) {
        net_log_thread_send_cmd(thread, &cmd);
    }
}

void net_log_schedule_resume_senders(net_log_schedule_t schedule) {
    struct net_log_thread_cmd cmd;
    cmd.m_size = sizeof(cmd);
    cmd.m_cmd = net_log_thread_cmd_resume;

    net_log_thread_t thread;
    TAILQ_FOREACH(thread, &schedule->m_threads, m_next) {
        net_log_thread_send_cmd(thread, &cmd);
    }
}

/* int net_log_schedule_start_main(net_log_schedule_t schedule) { */
/*     uint8_t need_mgr = 0; */
/*     uint8_t need_pipe = 0; */

/*     uint8_t i; */
/*     for(i = 0; i < schedule->m_category_count; ++i) { */
/*         net_log_category_t category = schedule->m_categories[i]; */
/*         if (category == NULL) continue; */

/*         if (category->m_sender == NULL) { */
/*             if (category->m_flusher == NULL) { */
/*                 need_mgr = 1; */
/*             } */
/*             else { */
/*                 need_pipe = 1; */
/*             } */
/*         } */
/*         else { */
/*             need_pipe = 1; */
/*         } */
/*     } */

/*     if (need_pipe) { */
/*         if (net_log_schedule_init_main_thread_pipe(schedule) != 0) { */
/*             CPE_ERROR(schedule->m_em, "log: schedule: init main thread pipe fail!"); */
/*             return -1; */
/*         } */
/*     } */
/*     else if (need_mgr) { */
/*         if (net_log_schedule_init_main_thread_mgr(schedule) != 0) { */
/*             CPE_ERROR(schedule->m_em, "log: schedule: init main thread mgr fail!"); */
/*             return -1; */
/*         } */
/*     } */

/*     return 0; */
/* } */

/* void net_log_schedule_stop_main(net_log_schedule_t schedule) { */
/*     if (schedule->m_main_thread_request_mgr) { */
/*         if (schedule->m_cfg_cache_dir) { /\*保存缓存 *\/ */
/*             net_log_request_mgr_save_and_clear_requests(schedule->m_main_thread_request_mgr); */
/*         } */

/*         if (schedule->m_main_thread_pipe */
/*             && schedule->m_main_thread_pipe->m_bind_request_mgr == schedule->m_main_thread_request_mgr) */
/*         { */
/*             schedule->m_main_thread_pipe->m_bind_request_mgr = NULL; */
/*         } */
        
/*         net_log_request_manage_free(schedule->m_main_thread_request_mgr); */
/*         schedule->m_main_thread_request_mgr = NULL; */
/*     } */

/*     if (schedule->m_main_thread_pipe) { */
/*         if (net_log_pipe_is_processing(schedule->m_main_thread_pipe)) { */
/*             net_log_pipe_stop_process(schedule->m_main_thread_pipe); */
/*         } */
        
/*         net_log_pipe_free(schedule->m_main_thread_pipe); */
/*         schedule->m_main_thread_pipe = NULL; */
/*     } */
/* } */

void net_log_state_fsm_notify_state_chagne(net_log_schedule_t schedule) {
    net_log_state_monitor_t monitor;

    TAILQ_FOREACH(monitor, &schedule->m_state_monitors, m_next) {
        monitor->m_monitor_fun(monitor->m_monitor_ctx, schedule);
    }
}
