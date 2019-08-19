#include "cpe/fsm/fsm_def.h"
#include "net_log_state_i.h"
#include "net_log_thread_i.h"
#include "net_log_thread_cmd.h"
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
    }
}

int net_log_schedule_start_threads(net_log_schedule_t schedule) {
    net_log_thread_t thread;

    TAILQ_FOREACH(thread, &schedule->m_threads, m_next) {
        if (net_log_thread_start(thread) != 0) goto START_ERROR;
        if (net_log_thread_update_net(thread) != 0) goto START_ERROR;
        if (net_log_thread_update_env(thread) != 0) goto START_ERROR;
    }

    return 0;

START_ERROR:
    if (net_log_schedule_notify_stop_threads(schedule)) {
        net_log_schedule_wait_stop_threads(schedule);
    }
    return -1;
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

void net_log_state_fsm_notify_state_chagne(net_log_schedule_t schedule) {
    net_log_state_monitor_t monitor;

    TAILQ_FOREACH(monitor, &schedule->m_state_monitors, m_next) {
        monitor->m_monitor_fun(monitor->m_monitor_ctx, schedule);
    }
}
