#ifndef NET_LOG_STATISTIC_MONITOR_I_H_INCLEDED
#define NET_LOG_STATISTIC_MONITOR_I_H_INCLEDED
#include "net_log_statistic_monitor.h"
#include "net_log_schedule_i.h"

struct net_log_statistic_monitor {
    net_log_schedule_t m_schedule;
    TAILQ_ENTRY(net_log_statistic_monitor) m_next;
    net_log_statistic_on_discard_fun_t m_on_discard;
    net_log_statistic_on_op_error_fun_t m_on_op_error;
    void * m_monitor_ctx;
};

#endif
