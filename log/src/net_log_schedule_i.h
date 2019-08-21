#ifndef NET_LOG_SCHEDULE_I_H_INCLEDED
#define NET_LOG_SCHEDULE_I_H_INCLEDED
#if NET_LOG_MULTI_THREAD
#include "pthread.h"
#endif
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/hash.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "cpe/utils/buffer.h"
#include "cpe/fsm/fsm_ins.h"
#include "net_trans_system.h"
#include "net_log_schedule.h"

#if NET_LOG_MULTI_THREAD
#define _MS(__sentence) __sentence
#define _SS(__sentence)
#else
#define _MS(__sentence)
#define _SS(__sentence) __sentence
#endif

NET_BEGIN_DECL

typedef TAILQ_HEAD(net_log_thread_list, net_log_thread) net_log_thread_list_t;
typedef TAILQ_HEAD(net_log_flusher_list, net_log_flusher) net_log_flusher_list_t;
typedef TAILQ_HEAD(net_log_sender_list, net_log_sender) net_log_sender_list_t;
typedef TAILQ_HEAD(net_log_category_list, net_log_category) net_log_category_list_t;
typedef TAILQ_HEAD(net_log_request_list, net_log_request) net_log_request_list_t;
typedef TAILQ_HEAD(net_log_request_cache_list, net_log_request_cache) net_log_request_cache_list_t;
typedef TAILQ_HEAD(net_log_state_monitor_list, net_log_state_monitor) net_log_state_monitor_list_t;
typedef TAILQ_HEAD(net_log_statistic_monitor_list, net_log_statistic_monitor) net_log_statistic_monitor_list_t;
typedef TAILQ_HEAD(net_log_env_list, net_log_env) net_log_env_list_t;

typedef struct net_log_request * net_log_request_t;
typedef struct net_log_request_cache * net_log_request_cache_t;
typedef struct net_log_request_param * net_log_request_param_t;
typedef struct net_log_thread_cmd * net_log_thread_cmd_t;
typedef struct net_log_builder * net_log_builder_t;
typedef struct net_log_group * net_log_group_t;
typedef struct net_log_tag * net_log_tag_t;
typedef struct net_log_buf * net_log_buf_t;
typedef struct net_log_category_cfg_tag * net_log_category_cfg_tag_t;
typedef struct net_log_lz4_buf * net_log_lz4_buf_t;

struct net_log_schedule {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    uint8_t m_debug;
    net_schedule_t m_net_schedule;
    net_driver_t m_net_driver;
    vfs_mgr_t m_vfs;
    char * m_cfg_project;
    char * m_cfg_access_id;
    char * m_cfg_access_key;
    char * m_cfg_source;
    uint8_t m_cfg_active_request_count;
    char * m_cfg_net_interface;
    char * m_cfg_remote_address;
    uint32_t m_cfg_connect_timeout_s;
    uint32_t m_cfg_send_timeout_s;
    uint32_t m_cfg_timeout_ms;
    uint32_t m_cfg_cache_mem_capacity;
    uint32_t m_cfg_cache_file_capacity;
    net_log_compress_type_t m_cfg_compress;
    char * m_cfg_cache_dir;
    uint32_t m_cfg_dump_span_ms;
    uint32_t m_cfg_stop_wait_ms;
    net_timer_t m_dump_timer;

    /**/
    net_local_ip_stack_monitor_t m_net_monitor;

    /*state*/
    fsm_def_machine_t m_state_fsm_def;
    struct fsm_machine m_state_fsm;
    net_log_state_monitor_list_t m_state_monitors;

    /*env*/
    net_log_env_list_t m_envs;
    net_log_env_t m_env_active;
    
    /*categories*/
    uint8_t m_category_count;
    net_log_category_t * m_categories;

    /*statistic*/
    uint32_t m_statistics_cache_created;
    uint32_t m_statistics_cache_destoried;
    uint32_t m_statistics_cache_discard;
    net_log_statistic_monitor_list_t m_statistic_monitors;
    
    /*threads*/
    uint8_t m_runing_thread_count;
    _MS(pthread_t m_main_thread_id);
    net_log_thread_t m_thread_main;
    net_log_thread_list_t m_threads;

    /*builder helper*/
    net_log_category_t m_current_category;
};

mem_buffer_t net_log_schedule_tmp_buffer(net_log_schedule_t schedule);

void net_log_schedule_check_stop_complete(net_log_schedule_t schedule);

void net_log_schedule_set_active_env(net_log_schedule_t schedule, net_log_env_t env);

NET_END_DECL

#endif
