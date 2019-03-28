#ifndef NET_LOG_FLUSHER_I_H_INCLEDED
#define NET_LOG_FLUSHER_I_H_INCLEDED
#include "net_log_category_i.h"

NET_BEGIN_DECL

struct net_log_flusher {
    net_log_category_t m_category;
    log_queue_t m_queue;
    pthread_t m_thread;
};

net_log_flusher_t net_log_flusher_create(net_log_category_t category);
void net_log_flusher_free(net_log_flusher_t flusher);

NET_END_DECL

#endif
