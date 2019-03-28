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
    log_queue_t m_queue;
    pthread_t m_thread;
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};

int net_log_sender_queue(net_log_sender_t sender, log_producer_send_param_t send_param);

NET_END_DECL

#endif
