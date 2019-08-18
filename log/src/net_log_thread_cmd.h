#ifndef NET_LOG_PIPE_CMD_I_H_INCLEDED
#define NET_LOG_PIPE_CMD_I_H_INCLEDED
#include "net_log_schedule_i.h"

NET_BEGIN_DECL

typedef enum net_log_thread_cmd_type {
    net_log_thread_cmd_type_package_pack,
    net_log_thread_cmd_type_package_send,
    net_log_thread_cmd_type_update_env,
    net_log_thread_cmd_type_update_net,
    net_log_thread_cmd_type_stop_begin,
    net_log_thread_cmd_type_stop_force,
    net_log_thread_cmd_type_stoped,
    net_log_thread_cmd_type_staistic_package_discard,
    net_log_thread_cmd_type_staistic_package_success,
} net_log_thread_cmd_type_t;

struct net_log_thread_cmd {
    uint8_t m_size;
    uint8_t m_cmd;
};

struct net_log_thread_cmd_update_env {
    struct net_log_thread_cmd head;
    net_log_env_t m_env;
};

struct net_log_thread_cmd_update_net {
    struct net_log_thread_cmd head;
    net_local_ip_stack_t m_local_ip_stack;
};

struct net_log_thread_cmd_stoped {
    struct net_log_thread_cmd head;
    net_log_thread_t log_thread;
};

struct net_log_thread_cmd_package_pack {
    struct net_log_thread_cmd head;
    net_log_builder_t m_builder;
};

struct net_log_thread_cmd_package_send {
    struct net_log_thread_cmd head;
    net_log_request_param_t m_send_param;
};

struct net_log_thread_cmd_staistic_load {
    struct net_log_thread_cmd head;
    uint32_t m_load_package_count;
    uint32_t m_load_record_count;
};

struct net_log_thread_cmd_staistic_package_success {
    struct net_log_thread_cmd head;
    net_log_category_t m_category;
};

struct net_log_thread_cmd_staistic_package_discard {
    struct net_log_thread_cmd head;
    net_log_category_t m_category;
    net_log_discard_reason_t m_reason;
};

NET_END_DECL

#endif

