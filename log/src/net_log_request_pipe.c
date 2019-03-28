#include <assert.h>
#include "net_log_request_pipe.h"

net_log_request_pipe_t net_log_request_pipe_create(net_log_schedule_t schedule) {
    net_log_request_pipe_t pipe = mem_alloc(schedule->m_alloc, sizeof(struct net_log_request_pipe));

    pipe->m_schedule = schedule;
    pthread_mutex_init(&pipe->m_mutex, NULL);
    pipe->m_bind_to = NULL;
    
    return pipe;
}

void net_log_request_pipe_free(net_log_request_pipe_t pipe) {
    net_log_schedule_t schedule = pipe->m_schedule;

    if (pipe->m_bind_to) {
        net_log_request_pipe_unbind(pipe);
    }
    
    pthread_mutex_destroy(&pipe->m_mutex);
    
    mem_free(schedule->m_alloc, pipe);
}

int net_log_request_pipe_bind(net_log_request_pipe_t pipe, net_log_request_manage_t mgr) {
    assert(pipe->m_bind_to == NULL);
    
    return 0;
}

void net_log_request_pipe_unbind(net_log_request_pipe_t pipe) {
    assert(pipe->m_bind_to != NULL);

}

int net_log_request_pipe_queue(net_log_request_pipe_t pipe, log_producer_send_param_t send_param) {
    net_log_schedule_t schedule = pipe->m_schedule;

    pthread_mutex_lock(&pipe->m_mutex);

    /* if (log_queue_push(pipe->m_queue, send_param) != 0) { */
    /*     CPE_ERROR(schedule->m_em, "log: pipe %s: push param fail", pipe->m_name); */
    /*     return -1; */
    /* } */
    /* else { */
    /*     pthread_cond_signal(&pipe->m_cond); */
    /* } */
    
    pthread_mutex_unlock(&pipe->m_mutex);

    return 0;
}

