#ifndef NET_LOG_REQUEST_PIPE_I_H_INCLEDED
#define NET_LOG_REQUEST_PIPE_I_H_INCLEDED
#include "net_log_schedule_i.h"

NET_BEGIN_DECL

struct net_log_pipe {
    net_log_schedule_t m_schedule;
    const char * m_name;

    _MS(pthread_mutex_t m_mutex);
    int m_pipe_fd[2];
    char m_pipe_r_buf[64];
    uint16_t m_pipe_r_size;
    net_log_queue_t m_send_param_queue;
    net_watcher_t m_bind_watcher;
    net_log_request_manage_t m_bind_request_mgr;
};

net_log_pipe_t net_log_pipe_create(net_log_schedule_t schedule, const char * name);
void net_log_pipe_free(net_log_pipe_t pipe);

int net_log_pipe_send_cmd(net_log_pipe_t pipe, net_log_pipe_cmd_t cmd);
int net_log_pipe_queue(net_log_pipe_t pipe, net_log_request_param_t send_param);
net_log_request_param_t net_log_pipe_pop(net_log_pipe_t pipe);

uint8_t net_log_pipe_is_processing(net_log_pipe_t pipe);
int net_log_pipe_begin_process(net_log_pipe_t pipe, net_driver_t net_driver);
void net_log_pipe_stop_process(net_log_pipe_t pipe);

void net_log_pipe_dispatch(net_log_pipe_t pipe, net_log_pipe_cmd_t cmd);

NET_END_DECL

#endif
