#ifndef NET_LOG_REQUEST_H_INCLEDED
#define NET_LOG_REQUEST_H_INCLEDED
#include "net_log_request_manage.h"

#define LOG_SEND_OK 0
#define LOG_SEND_NETWORK_ERROR 1
#define LOG_SEND_QUOTA_EXCEED 2
#define LOG_SEND_UNAUTHORIZED 3
#define LOG_SEND_SERVER_ERROR 4
#define LOG_SEND_DISCARD_ERROR 5
#define LOG_SEND_TIME_ERROR 6

struct net_log_request_param {
    log_producer_config_t producer_config;
    void * producer_manager;
    lz4_log_buf * log_buf;
    uint32_t magic_num;
    uint32_t builder_time;
};

struct net_log_request {
    net_log_request_manage_t m_mgr;
    TAILQ_ENTRY(net_log_request) m_next;
    CURL * m_handler;
    net_watcher_t m_watcher;
    net_log_category_t m_category;
};

net_log_request_t net_log_request_create(net_log_request_manage_t mgr, net_log_request_param_t send_param);
void net_log_request_free(net_log_request_t request);

void net_log_request_real_free(net_log_request_t request);

net_log_request_param_t create_net_log_request_param(
    log_producer_config_t producer_config,
    void * producer_manager,
    lz4_log_buf * log_buf,
    uint32_t builder_time);

void net_log_request_param_free(net_log_request_param_t send_param);

#endif
