#ifndef NET_LOG_REQUEST_H_INCLEDED
#define NET_LOG_REQUEST_H_INCLEDED
#include "net_log_request_manage.h"

#define LOG_PRODUCER_SEND_MAGIC_NUM 0x1B35487A

typedef enum net_log_request_send_result {
    net_log_request_send_ok = 0,
    net_log_request_send_network_error = 1,
    net_log_request_send_quota_exceed = 2,
    net_log_request_send_unauthorized = 3,
    net_log_request_send_server_error = 4,
    net_log_request_send_discard_error = 5,
    net_log_request_send_time_error = 6,
} net_log_request_send_result_t;

typedef enum net_log_request_complete_state {
    net_log_request_complete_done,
    net_log_request_complete_cancel,
    net_log_request_complete_timeout,
} net_log_request_complete_state_t;

struct net_log_request_param {
    net_log_category_t category;
    lz4_log_buf_t log_buf;
    uint32_t magic_num;
    uint32_t builder_time;
};

struct net_log_request {
    net_log_request_manage_t m_mgr;
    TAILQ_ENTRY(net_log_request) m_next;
    uint32_t m_id;
    CURL * m_handler;
    net_watcher_t m_watcher;
    net_log_category_t m_category;

    /*result*/
    net_log_request_param_t m_send_param;
    net_log_request_send_result_t m_last_send_error;
    int32_t m_last_sleep_ms;
    int32_t m_first_error_time;
};

net_log_request_t net_log_request_create(net_log_request_manage_t mgr, net_log_request_param_t send_param);
void net_log_request_free(net_log_request_t request);

void net_log_request_real_free(net_log_request_t request);

void net_log_request_complete(
    net_log_schedule_t schedule, net_log_request_t request, net_log_request_complete_state_t complete_state);

const char * net_log_request_complete_state_str(net_log_request_complete_state_t state);

/**/
net_log_request_param_t
net_log_request_param_create(
    net_log_category_t category, lz4_log_buf_t log_buf, uint32_t builder_time);

void net_log_request_param_free(net_log_request_param_t send_param);

#endif
