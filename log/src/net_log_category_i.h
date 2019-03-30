#ifndef NET_LOG_CATEGORY_I_H_INCLEDED
#define NET_LOG_CATEGORY_I_H_INCLEDED
#include "net_log_category.h"
#include "net_log_schedule_i.h"

NET_BEGIN_DECL

typedef struct net_log_category_cfg_tag {
    char * m_key;
    char * m_value;
} * net_log_category_cfg_tag_t;

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
    net_log_compress_type_t m_cfg_compress;
    uint32_t m_cfg_timeout_ms;
    uint32_t m_cfg_count_per_package;
    uint32_t m_cfg_bytes_per_package;
    uint32_t m_cfg_max_buffer_bytes;
    uint32_t m_cfg_connect_timeout_s;
    uint32_t m_cfg_send_timeout_s;
    /* uint32_t m_cfg_destroyFlusherWaitTimeoutSec; */
    /* int32_t destroySenderWaitTimeoutSec; */
    
    /*runtime*/
    volatile uint32_t m_networkRecover;
    volatile uint32_t m_totalBufferSize;
    net_log_group_builder_t m_builder;
    int32_t m_firstLogTime;
    char * m_pack_prefix;
    volatile uint32_t m_pack_index;
};

void net_log_category_network_recover(net_log_category_t category);

net_log_request_param_t net_log_category_build_request(net_log_category_t category, net_log_group_builder_t builder);
int net_log_category_commit_request(net_log_category_t category, net_log_request_param_t send_param, uint8_t in_main_thread);

int net_log_category_add_log(
    net_log_category_t category, int32_t pair_count, char ** keys, size_t * key_lens, char ** values, size_t * val_lens);

NET_END_DECL

#endif

