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
typedef struct net_log_category * net_log_category_t;
typedef struct net_log_flusher * net_log_flusher_t;
typedef struct net_log_sender * net_log_sender_t;

NET_END_DECL

#endif
