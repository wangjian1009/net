#ifndef NET_LOG_SYSTEM_H_INCLEDED
#define NET_LOG_SYSTEM_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef enum net_log_compress_type {
    net_log_compress_none,
    net_log_compress_lz4,
} net_log_compress_type_t;

typedef enum net_log_schedule_state {
    net_log_schedule_state_init,
    net_log_schedule_state_runing,
    net_log_schedule_state_pause,
    net_log_schedule_state_stoping,
    net_log_schedule_state_error,
} net_log_schedule_state_t;

typedef struct net_log_schedule * net_log_schedule_t;
typedef struct net_log_thread * net_log_thread_t;
typedef struct net_log_thread_processor * net_log_thread_processor_t;
typedef struct net_log_category * net_log_category_t;
typedef struct net_log_state_monitor * net_log_state_monitor_t;
typedef struct net_log_statistic_monitor * net_log_statistic_monitor_t;

typedef struct net_log_env * net_log_env_t;
typedef struct net_log_env_it * net_log_env_it_t;

typedef enum net_log_discard_reason {
    net_log_discard_reason_pack_fail,
    net_log_discard_reason_queue_to_pack_fail,
    net_log_discard_reason_queue_to_send_fail,
} net_log_discard_reason_t;

#define net_log_discard_reason_count (net_log_discard_reason_queue_to_pack_fail + 1)

NET_END_DECL

#endif
