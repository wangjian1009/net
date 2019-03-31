#ifndef NET_LOG_CATEGORY_I_H_INCLEDED
#define NET_LOG_CATEGORY_I_H_INCLEDED
#include "cpe/utils/traffic_bps.h"
#include "net_log_category.h"
#include "net_log_schedule_i.h"

NET_BEGIN_DECL

struct net_log_category_cfg_tag {
    char * m_key;
    char * m_value;
};

struct net_log_category {
    net_log_schedule_t m_schedule;
    net_log_flusher_t m_flusher;
    TAILQ_ENTRY(net_log_category) m_next_for_flusher;
    net_log_sender_t m_sender;
    TAILQ_ENTRY(net_log_category) m_next_for_sender;
    char m_name[64];
    uint8_t m_id;

    /*config*/
    char * m_cfg_topic;
    net_log_category_cfg_tag_t m_cfg_tags;
    uint16_t m_cfg_tag_count;
    uint16_t m_cfg_tag_capacity;
    uint32_t m_cfg_count_per_package;
    uint32_t m_cfg_bytes_per_package;
    
    /*runtime*/
    net_timer_t m_commit_timer;
    net_log_builder_t m_builder;
    char * m_pack_prefix;
    volatile uint32_t m_pack_index;

    /*statistics*/
    uint32_t m_statistics_log_count;
    uint32_t m_statistics_package_count;
    struct cpe_traffic_bps m_statistics_input_bps;

    pthread_mutex_t m_statistics_mutex;
    uint32_t m_statistics_fail_log_count;    
    uint32_t m_statistics_fail_package_count;    
    struct cpe_traffic_bps m_statistics_output_bps;
};

void net_log_category_network_recover(net_log_category_t category);

net_log_request_param_t net_log_category_build_request(net_log_category_t category, net_log_builder_t builder);
int net_log_category_commit_request(net_log_category_t category, net_log_request_param_t send_param, uint8_t in_main_thread);

void net_log_category_log_begin(net_log_category_t category);
void net_log_category_log_apppend(net_log_category_t category, const char * key, const char * value);
void net_log_category_log_end(net_log_category_t category);

void net_log_category_add_fail_statistics(net_log_category_t category);

NET_END_DECL

#endif

