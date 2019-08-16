#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/fsm/fsm_def.h"
#include "net_schedule.h"
#include "net_log_state_i.h"

static void net_log_state_fsm_init_enter(fsm_machine_t fsm, fsm_def_state_t state, void * event) {
    net_log_schedule_t schedule = fsm_machine_context(fsm);
    net_log_schedule_wait_stop_threads(schedule);
    net_log_state_fsm_notify_state_chagne(schedule);
}

static void net_log_state_fsm_init_leave(fsm_machine_t fsm, fsm_def_state_t state, void * event) {
}

static uint32_t net_log_state_fsm_init_trans(fsm_machine_t fsm, fsm_def_state_t state, void * input_evt) {
    net_log_schedule_t schedule = fsm_machine_context(fsm);
    net_log_state_fsm_evt_t evt = input_evt;

    switch(evt->m_type) {
    case net_log_state_fsm_evt_start:
        if (net_log_schedule_start_threads(schedule) != 0) {
            return net_log_schedule_state_error;
        }

        if (schedule->m_env_active == NULL) {
            if (schedule->m_debug) {
                CPE_INFO(schedule->m_em, "log: schedule: state-fsm: init: no cfg-ep, auto pause!");
            }
            return net_log_schedule_state_pause;
        } else if (net_schedule_local_ip_stack(schedule->m_net_schedule) == net_local_ip_stack_none) {
            if (schedule->m_debug) {
                CPE_INFO(schedule->m_em, "log: schedule: state-fsm: init: no active network, auto pause!");
            }
            return net_log_schedule_state_pause;
        } else {
            net_log_schedule_resume_senders(schedule);
            return net_log_schedule_state_runing;
        }
    case net_log_state_fsm_evt_stop_begin:
    case net_log_state_fsm_evt_stop_complete:
    case net_log_state_fsm_evt_pause:
    case net_log_state_fsm_evt_resume:
        return FSM_KEEP_STATE;
    }
}

int net_log_state_fsm_create_init(fsm_def_machine_t fsm_def, error_monitor_t em) {
    fsm_def_state_t s = fsm_def_state_create_ex(
        fsm_def, net_log_schedule_state_str(net_log_schedule_state_init), net_log_schedule_state_init);
    if (s == NULL) {
        CPE_ERROR(em, "log: schedule: state-fsm: init: create state fail!");
        return -1;
    }

    fsm_def_state_set_action(s, net_log_state_fsm_init_enter, net_log_state_fsm_init_leave);

    if (fsm_def_state_add_transition(s, net_log_state_fsm_init_trans) != 0) {
        CPE_ERROR(em, "log: schedule: state-fsm: init: add trans fail!");
        fsm_def_state_free(s);
        return -1;
    }

    return 0;
}
