#ifndef NET_LOG_SCHEDULE_I_H_INCLEDED
#define NET_LOG_SCHEDULE_I_H_INCLEDED
#include "pthread.h"
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/hash.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "cpe/utils/buffer.h"
#include "net_log_schedule.h"
#include "log_api.h"

NET_BEGIN_DECL

typedef struct net_log_flusher * net_log_flusher_t;
typedef struct _log_queue * log_queue_t;
typedef struct _log_producer_config * log_producer_config_t;
typedef struct _log_producer_manager * log_producer_manager_t;

struct net_log_schedule {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    uint8_t m_debug;
    net_schedule_t m_net_schedule;
    net_driver_t m_net_driver;
    char * m_cfg_project;
    char * m_cfg_ep;
    char * m_cfg_access_id;
    char * m_cfg_access_key;

    /*state*/
    volatile net_log_schedule_state_t m_state;
    
    /*categories*/
    uint8_t m_category_count;
    net_log_category_t * m_categories;

    /*builder helper*/
    net_log_category_t m_current_category;
    int32_t m_kv_count;
    int32_t m_kv_capacity;
    char * * m_keys;
    size_t * m_keys_len;
    char * * m_values;
    size_t * m_values_len;
    struct mem_buffer m_kv_buffer;
};

mem_buffer_t net_log_schedule_tmp_buffer(net_log_schedule_t schedule);

NET_END_DECL

#endif
