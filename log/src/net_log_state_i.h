#ifndef NET_LOG_STATE_H_INCLEDED
#define NET_LOG_STATE_H_INCLEDED
#include "net_log_schedule_i.h"

typedef enum net_log_state_fsm_evt_type {
    net_log_state_fsm_evt_start,
    net_log_state_fsm_evt_stop,
    net_log_state_fsm_evt_pause,
    net_log_state_fsm_evt_resume,
} net_log_state_fsm_evt_type_t;

typedef struct net_log_state_fsm_evt {
    net_log_state_fsm_evt_type_t m_type;
} * net_log_state_fsm_evt_t;

fsm_def_machine_t net_log_create_fsm_def(net_log_schedule_t schedule);

int net_log_state_fsm_create_init(fsm_def_machine_t fsm_def, error_monitor_t em);
int net_log_state_fsm_create_runing(fsm_def_machine_t fsm_def, error_monitor_t em);
int net_log_state_fsm_create_pause(fsm_def_machine_t fsm_def, error_monitor_t em);
int net_log_state_fsm_create_stoping(fsm_def_machine_t fsm_def, error_monitor_t em);
int net_log_state_fsm_create_error(fsm_def_machine_t fsm_def, error_monitor_t em);

int net_log_state_fsm_apply_evt(net_log_schedule_t schedule, net_log_state_fsm_evt_type_t type);

int net_log_schedule_start_threads(net_log_schedule_t schedule);
void net_log_schedule_stop_threads(net_log_schedule_t schedule);
void net_log_schedule_pause_senders(net_log_schedule_t schedule);
void net_log_schedule_resume_senders(net_log_schedule_t schedule);

#endif
