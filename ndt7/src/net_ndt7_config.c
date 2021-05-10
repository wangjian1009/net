#include "net_ndt7_config.h"

void net_ndt7_config_init_default(net_ndt7_config_t config) {
    config->m_progress_interval_ms = 250;
    config->m_upload.m_duration_ms = 20 * 1000;     /*20秒*/
    config->m_upload.m_max_message_size = 16777216; /* (1<<24) = 16MB */
    config->m_upload.m_min_message_size = 8192;     /* (1<<13) */
    config->m_upload.m_max_queue_size = 16777216;   /* 16MB */
}

