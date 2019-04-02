#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/fsm/fsm_def.h"
#include "net_log_state_i.h"

static void net_log_state_fsm_runing_enter(fsm_machine_t fsm, fsm_def_state_t state, void * event) {
}

static void net_log_state_fsm_runing_leave(fsm_machine_t fsm, fsm_def_state_t state, void * event) {
}

static uint32_t net_log_state_fsm_runing_trans(fsm_machine_t fsm, fsm_def_state_t state, void * input_evt) {
    net_log_schedule_t schedule = fsm_machine_context(fsm);
    net_log_state_fsm_evt_t evt = input_evt;

    switch(evt->m_type) {
    case net_log_state_fsm_evt_stop:
        net_log_schedule_stop_threads(schedule);
        return net_log_schedule_state_init;
    case net_log_state_fsm_evt_pause:
        net_log_schedule_pause_senders(schedule);
        return net_log_schedule_state_pause;
    case net_log_state_fsm_evt_start:
    case net_log_state_fsm_evt_resume:
        return FSM_KEEP_STATE;
    }
}

int net_log_state_fsm_create_runing(fsm_def_machine_t fsm_def, error_monitor_t em) {
    fsm_def_state_t s = fsm_def_state_create_ex(
        fsm_def, net_log_schedule_state_str(net_log_schedule_state_runing), net_log_schedule_state_runing);
    if (s == NULL) {
        CPE_ERROR(em, "log: schedule: state-fsm: runing: create state fail!");
        return -1;
    }

    fsm_def_state_set_action(s, net_log_state_fsm_runing_enter, net_log_state_fsm_runing_leave);

    if (fsm_def_state_add_transition(s, net_log_state_fsm_runing_trans) != 0) {
        CPE_ERROR(em, "log: schedule: state-fsm: runing: add trans fail!");
        fsm_def_state_free(s);
        return -1;
    }

    return 0;
}