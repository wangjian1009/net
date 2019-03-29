#include <assert.h>
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_errno.h"
#include "cpe/pal/pal_unistd.h"
#include "cpe/pal/pal_string.h"
#include "net_watcher.h"
#include "net_log_request_pipe.h"
#include "net_log_request_manage.h"

static void net_log_request_pipe_action(void * ctx, int fd, uint8_t do_read, uint8_t do_write);

net_log_request_pipe_t net_log_request_pipe_create(net_log_schedule_t schedule) {
    net_log_request_pipe_t request_pipe = mem_alloc(schedule->m_alloc, sizeof(struct net_log_request_pipe));

    request_pipe->m_schedule = schedule;
    pthread_mutex_init(&request_pipe->m_mutex, NULL);
    request_pipe->m_bind_watcher = NULL;
    request_pipe->m_bind_to = NULL;

    if (pipe(request_pipe->m_pipe_fd) != 0) {
        CPE_ERROR(schedule->m_em, "log: pipe: create pipe fail, error=%d (%s)!", errno, strerror(errno));
        mem_free(schedule->m_alloc, request_pipe);
        return NULL;
    }

    if (cpe_sock_set_none_block(request_pipe->m_pipe_fd[0], 1) != 0) {
        CPE_ERROR(schedule->m_em, "log: pipe: set none block fail, error=%d (%s)!", errno, strerror(errno));
        close(request_pipe->m_pipe_fd[0]);
        close(request_pipe->m_pipe_fd[1]);
        mem_free(schedule->m_alloc, request_pipe);
        return NULL;
    }
    
    return request_pipe;
}

void net_log_request_pipe_free(net_log_request_pipe_t pipe) {
    net_log_schedule_t schedule = pipe->m_schedule;

    if (pipe->m_bind_to) {
        net_log_request_pipe_unbind(pipe);
    }
    assert(pipe->m_bind_to == NULL);
    assert(pipe->m_bind_watcher == NULL);

    close(pipe->m_pipe_fd[0]);
    close(pipe->m_pipe_fd[1]);
    pipe->m_pipe_fd[0] = -1;
    pipe->m_pipe_fd[1] = -1;
    
    pthread_mutex_destroy(&pipe->m_mutex);
    
    mem_free(schedule->m_alloc, pipe);
}

int net_log_request_pipe_bind(net_log_request_pipe_t pipe, net_log_request_manage_t mgr) {
    net_log_schedule_t schedule = pipe->m_schedule;

    if (pipe->m_bind_to == NULL) {
        CPE_ERROR(schedule->m_em, "log: pipe: bind: already binded!");
        return -1;
    }

    assert(pipe->m_bind_watcher == NULL);
    pipe->m_bind_watcher = net_watcher_create(mgr->m_net_driver, pipe->m_pipe_fd[0], pipe, net_log_request_pipe_action);
    if (pipe->m_bind_watcher == NULL) {
        CPE_ERROR(schedule->m_em, "log: pipe: bind: create watcher fail!");
        return -1;
    }

    pipe->m_bind_to = mgr;
    
    return 0;
}

void net_log_request_pipe_unbind(net_log_request_pipe_t pipe) {
    assert(pipe->m_bind_to != NULL);
    assert(pipe->m_bind_watcher != NULL);

    net_watcher_free(pipe->m_bind_watcher);
    pipe->m_bind_watcher = NULL;
    pipe->m_bind_to = NULL;
}

int net_log_request_pipe_queue(net_log_request_pipe_t pipe, net_log_request_param_t send_param) {
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

static void net_log_request_pipe_action(void * ctx, int fd, uint8_t do_read, uint8_t do_write) {
    if (do_read) {
    }
}
