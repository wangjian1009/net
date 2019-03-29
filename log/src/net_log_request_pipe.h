#ifndef NET_LOG_REQUEST_I_H_INCLEDED
#define NET_LOG_REQUEST_I_H_INCLEDED
#include "net_log_schedule_i.h"

NET_BEGIN_DECL

struct net_log_request_pipe {
    net_log_schedule_t m_schedule;
    pthread_mutex_t m_mutex;
    int m_pipe_fd[2];
    net_watcher_t m_bind_watcher;
    net_log_request_manage_t m_bind_to;
};

net_log_request_pipe_t net_log_request_pipe_create(net_log_schedule_t schedule);
void net_log_request_pipe_free(net_log_request_pipe_t pipe);

int net_log_request_pipe_queue(net_log_request_pipe_t pipe, net_log_request_param_t send_param);

int net_log_request_pipe_bind(net_log_request_pipe_t pipe, net_log_request_manage_t mgr);
void net_log_request_pipe_unbind(net_log_request_pipe_t pipe);

NET_END_DECL

#endif
