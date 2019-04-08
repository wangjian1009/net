#ifndef NET_LOG_STATE_MONITOR_I_H_INCLEDED
#define NET_LOG_STATE_MONITOR_I_H_INCLEDED
#include "net_log_state_monitor.h"
#include "net_log_schedule_i.h"

struct net_log_state_monitor {
    net_log_schedule_t m_schedule;
    TAILQ_ENTRY(net_log_state_monitor) m_next;
    net_log_state_monitor_fun_t m_monitor_fun;
    void * m_monitor_ctx;
};

#endif
