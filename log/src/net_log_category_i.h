#ifndef NET_LOG_CATEGORY_I_H_INCLEDED
#define NET_LOG_CATEGORY_I_H_INCLEDED
#include "net_log_category.h"
#include "net_log_schedule_i.h"

NET_BEGIN_DECL

struct net_log_category {
    net_log_schedule_t m_schedule;
    net_log_flusher_t m_flusher;
    TAILQ_ENTRY(net_log_category) m_next_for_flusher;
    char m_name[64];
    uint8_t m_id;
    log_producer_manager_t m_producer_manager;
    log_producer_config_t m_producer_config;
};

void net_log_category_network_recover(net_log_category_t category);

int net_log_category_start(net_log_category_t category);
void net_log_category_notify_stop(net_log_category_t category);
void net_log_category_wait_stop(net_log_category_t category);

NET_END_DECL

#endif

