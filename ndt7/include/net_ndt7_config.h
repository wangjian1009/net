#ifndef NET_NDT7_CONFIG_H_INCLEDED
#define NET_NDT7_CONFIG_H_INCLEDED
#include "net_ndt7_system.h"

struct net_ndt7_config {
    int64_t m_progress_interval_ms;

    struct {
        int64_t m_duration_ms;
        uint32_t m_max_message_size;
        uint32_t m_min_message_size;
        uint32_t m_max_queue_size;
    } m_upload;
};

void net_ndt7_config_init_default(net_ndt7_config_t config);
    
#endif
