#ifndef NET_LOG_FLUSHER_I_H_INCLEDED
#define NET_LOG_FLUSHER_I_H_INCLEDED
#include "net_log_flusher.h"
#include "net_log_schedule_i.h"

NET_BEGIN_DECL

struct net_log_flusher {
    net_log_schedule_t m_schedule;
    TAILQ_ENTRY(net_log_flusher) m_next;
    net_log_category_list_t m_categories;
    char m_name[64];
    net_log_queue_t m_queue;
    pthread_t * m_thread;
    pthread_mutex_t m_mutex;
    pthread_cond_t m_cond;
};

int net_log_flusher_queue(net_log_flusher_t flusher, net_log_group_builder_t builder);

int net_log_flusher_start(net_log_flusher_t flusher);
void net_log_flusher_notify_stop(net_log_flusher_t flusher);
void net_log_flusher_wait_stop(net_log_flusher_t flusher);

NET_END_DECL

#endif
