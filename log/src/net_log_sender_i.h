#ifndef NET_LOG_SENDER_I_H_INCLEDED
#define NET_LOG_SENDER_I_H_INCLEDED
#include "net_log_sender.h"
#include "net_log_schedule_i.h"

NET_BEGIN_DECL

struct net_log_sender {
    net_log_schedule_t m_schedule;
    TAILQ_ENTRY(net_log_sender) m_next;
    net_log_category_list_t m_categories;
    char m_name[64];
    uint16_t m_cfg_active_request_count;
    net_log_pipe_t m_pipe;
    _MS(pthread_t * m_thread);
};

int net_log_sender_start(net_log_sender_t sender);
void net_log_sender_notify_stop(net_log_sender_t sender);
void net_log_sender_wait_stop(net_log_sender_t sender);

NET_END_DECL

#endif
