#ifndef NET_LOG_REQUEST_H_INCLEDED
#define NET_LOG_REQUEST_H_INCLEDED
#include "net_log_request_manage.h"

typedef enum net_log_request_send_result {
    net_log_request_send_ok = 0,
    net_log_request_send_network_error = 1,
    net_log_request_send_quota_exceed = 2,
    net_log_request_send_unauthorized = 3,
    net_log_request_send_server_error = 4,
    net_log_request_send_discard_error = 5,
    net_log_request_send_time_error = 6,
} net_log_request_send_result_t;

struct net_log_request_param {
    net_log_category_t category;
    lz4_log_buf_t log_buf;
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

void net_log_request_complete(net_log_request_t request);
void net_log_request_timeout(net_log_request_t request);
void net_log_request_cancel(net_log_request_t request);

/**/
net_log_request_param_t
create_net_log_request_param(
    net_log_category_t category, lz4_log_buf_t log_buf, uint32_t builder_time);

void net_log_request_param_free(net_log_request_param_t send_param);

#endif
