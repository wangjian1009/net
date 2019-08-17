#ifndef NET_LOG_REQUEST_H_INCLEDED
#define NET_LOG_REQUEST_H_INCLEDED
#include "net_log_thread_i.h"

#define LOG_PRODUCER_SEND_MAGIC_NUM 0x1B35487A

struct net_log_lz4_buf {
    net_log_schedule_t m_schedule;
    uint32_t length;
    uint32_t raw_length;
    unsigned char data[0];
};

typedef enum net_log_request_state {
    net_log_request_state_waiting,
    net_log_request_state_active,
} net_log_request_state_t;

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
    net_log_lz4_buf_t log_buf;
    uint32_t log_count;
    uint32_t magic_num;
    uint32_t builder_time;
};

struct net_log_request {
    net_log_thread_t m_thread;
    TAILQ_ENTRY(net_log_request) m_next;
    uint32_t m_id;
    net_log_category_t m_category;
    net_log_request_state_t m_state;
    net_trans_task_t m_task;
    net_timer_t m_delay_process;

    /*result*/
    net_log_request_param_t m_send_param;
    uint8_t m_response_have_request_id;
    net_log_request_send_result_t m_last_send_error;
    uint32_t m_last_sleep_ms;
    uint32_t m_first_error_time;
};

net_log_request_t net_log_request_create(net_log_thread_t thread, net_log_request_param_t send_param);
void net_log_request_free(net_log_request_t request);

void net_log_request_real_free(net_log_request_t request);

void net_log_request_active(net_log_request_t request);
void net_log_request_set_state(net_log_request_t request, net_log_request_state_t state);

const char * net_log_request_send_result_str(net_log_request_send_result_t result);
const char * net_log_request_state_str(net_log_request_state_t state);

/**/
net_log_request_param_t
net_log_request_param_create(
    net_log_category_t category, net_log_lz4_buf_t log_buf, uint32_t log_count, uint32_t builder_time);

void net_log_request_param_free(net_log_request_param_t send_param);

#endif
