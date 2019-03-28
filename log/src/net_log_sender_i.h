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
    net_log_request_pipe_t m_request_pipe;
    pthread_t * m_thread;
};

NET_END_DECL

#endif
