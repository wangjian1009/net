#ifndef NET_STATISTICS_BACKEND_I_H_INCLEDED
#define NET_STATISTICS_BACKEND_I_H_INCLEDED
#include "net_statistics_backend.h"
#include "net_statistics_i.h"

CPE_BEGIN_DECL

struct net_statistics_backend {
    net_statistics_t m_statistics;
    TAILQ_ENTRY(net_statistics_backend) m_next_for_statistics;
    char m_name[32];
    uint16_t m_backend_capacity;
    net_statistics_backend_init_fun_t m_backend_init;
    net_statistics_backend_fini_fun_t m_backend_fini;
    net_statistics_log_event_fun_t m_log_event;
    net_statistics_log_metric_for_count_fun_t m_log_metric_for_count;
    net_statistics_log_metric_for_duration_fun_t m_log_metric_for_duration;
};

CPE_END_DECL

#endif

