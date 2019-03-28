#ifndef NET_LOG_CATEGORY_I_H_INCLEDED
#define NET_LOG_CATEGORY_I_H_INCLEDED
#include "net_log_category.h"
#include "net_log_schedule_i.h"

NET_BEGIN_DECL

struct net_log_category {
    net_log_schedule_t m_schedule;
    net_log_flusher_t m_flusher;
    TAILQ_ENTRY(net_log_category) m_next_for_flusher;
    net_log_sender_t m_sender;
    TAILQ_ENTRY(net_log_category) m_next_for_sender;
    char m_name[64];
    uint8_t m_id;
    log_producer_manager_t m_producer_manager;
    log_producer_config_t m_producer_config;
};

void net_log_category_network_recover(net_log_category_t category);

log_producer_send_param_t net_log_category_build_request(net_log_category_t category, log_group_builder_t builder);
int net_log_category_commit_request(net_log_category_t category, log_producer_send_param_t send_param, uint8_t in_main_thread);

NET_END_DECL

#endif

