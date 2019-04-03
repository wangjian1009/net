#include <assert.h>
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_errno.h"
#include "cpe/pal/pal_unistd.h"
#include "cpe/pal/pal_string.h"
#include "net_watcher.h"
#include "net_log_pipe.h"
#include "net_log_pipe_cmd.h"
#include "net_log_request_manage.h"
#include "net_log_queue.h"
#include "net_log_category_i.h"
#include "net_log_request.h"

static void net_log_pipe_action(void * ctx, int fd, uint8_t do_read, uint8_t do_write);

net_log_pipe_t net_log_pipe_create(net_log_schedule_t schedule, const char * name) {
    net_log_pipe_t request_pipe = mem_alloc(schedule->m_alloc, sizeof(struct net_log_pipe));

    request_pipe->m_schedule = schedule;
    request_pipe->m_name = name;
    pthread_mutex_init(&request_pipe->m_mutex, NULL);
    request_pipe->m_send_param_queue = net_log_queue_create(32);
    request_pipe->m_bind_watcher = NULL;
    request_pipe->m_bind_request_mgr = NULL;
    request_pipe->m_pipe_r_size = 0;

    if (pipe(request_pipe->m_pipe_fd) != 0) {
        CPE_ERROR(schedule->m_em, "log: pipe %s: create pipe fail, error=%d (%s)!", name, errno, strerror(errno));
        net_log_queue_free(request_pipe->m_send_param_queue);
        mem_free(schedule->m_alloc, request_pipe);
        return NULL;
    }

    if (cpe_sock_set_none_block(request_pipe->m_pipe_fd[0], 1) != 0) {
        CPE_ERROR(schedule->m_em, "log: pipe %s: set none block fail, error=%d (%s)!", name, errno, strerror(errno));
        close(request_pipe->m_pipe_fd[0]);
        close(request_pipe->m_pipe_fd[1]);
        net_log_queue_free(request_pipe->m_send_param_queue);
        mem_free(schedule->m_alloc, request_pipe);
        return NULL;
    }
    
    return request_pipe;
}

void net_log_pipe_free(net_log_pipe_t pipe) {
    net_log_schedule_t schedule = pipe->m_schedule;

    assert(pipe->m_bind_request_mgr == NULL);
    assert(pipe->m_bind_watcher == NULL);

    close(pipe->m_pipe_fd[0]);
    close(pipe->m_pipe_fd[1]);
    pipe->m_pipe_fd[0] = -1;
    pipe->m_pipe_fd[1] = -1;

    net_log_queue_free(pipe->m_send_param_queue);
    
    pthread_mutex_destroy(&pipe->m_mutex);
    
    mem_free(schedule->m_alloc, pipe);
}

uint8_t net_log_pipe_is_processing(net_log_pipe_t pipe) {
    return pipe->m_bind_watcher == NULL ? 0 : 1;
}

int net_log_pipe_begin_process(net_log_pipe_t pipe, net_driver_t net_driver) {
    net_log_schedule_t schedule = pipe->m_schedule;

    if (pipe->m_bind_watcher != NULL) {
        CPE_ERROR(schedule->m_em, "log: pipe %s: bind: already binded!", pipe->m_name);
        return -1;
    }

    pipe->m_bind_watcher = net_watcher_create(net_driver, pipe->m_pipe_fd[0], pipe, net_log_pipe_action);
    if (pipe->m_bind_watcher == NULL) {
        CPE_ERROR(schedule->m_em, "log: pipe %s: bind: create watcher fail!", pipe->m_name);
        return -1;
    }

    net_watcher_update(pipe->m_bind_watcher, 1, 0);

    return 0;
}

void net_log_pipe_stop_process(net_log_pipe_t pipe) {
    assert(pipe->m_bind_watcher != NULL);

    net_watcher_free(pipe->m_bind_watcher);
    pipe->m_bind_watcher = NULL;
}

int net_log_pipe_queue(net_log_pipe_t pipe, net_log_request_param_t send_param) {
    net_log_schedule_t schedule = pipe->m_schedule;
    net_log_category_t category = send_param->category;

    pthread_mutex_lock(&pipe->m_mutex);

    struct net_log_pipe_cmd cmd;
    cmd.m_size = sizeof(cmd);
    cmd.m_cmd = net_log_pipe_cmd_send;

    if (write(pipe->m_pipe_fd[1], &cmd, cmd.m_size) < 0) {
        CPE_ERROR(
            schedule->m_em, "log: category [%d]%s: pipe %s: send notify cmd fail",
            category->m_id, category->m_name, pipe->m_name);
        pthread_mutex_unlock(&pipe->m_mutex);
        return -1;
    }

    if (net_log_queue_push(pipe->m_send_param_queue, send_param) != 0) {
        CPE_ERROR(
            schedule->m_em, "log: category [%d]%s: pipe %s: push param fail",
            category->m_id, category->m_name, pipe->m_name);
        pthread_mutex_unlock(&pipe->m_mutex);
        return -1;
    }

    pthread_mutex_unlock(&pipe->m_mutex);

    return 0;
}

int net_log_pipe_send_cmd(net_log_pipe_t pipe, net_log_pipe_cmd_t cmd) {
    pthread_mutex_lock(&pipe->m_mutex);
    
    if (write(pipe->m_pipe_fd[1], cmd, cmd->m_size) < 0) {
        CPE_ERROR(
            pipe->m_schedule->m_em, "log: pipe %s: write pipe fail, error=%d (%s)!",
            pipe->m_name, errno, strerror(errno));
        pthread_mutex_unlock(&pipe->m_mutex);
        return -1;
    }

    pthread_mutex_unlock(&pipe->m_mutex);
    return 0;
}

static net_log_request_param_t net_log_pipe_pop_send_param(net_log_pipe_t pipe) {
    net_log_request_param_t send_param;

    pthread_mutex_lock(&pipe->m_mutex);
    send_param = net_log_queue_pop(pipe->m_send_param_queue);
    pthread_mutex_unlock(&pipe->m_mutex);

    return send_param;
}

static void net_log_pipe_action(void * ctx, int fd, uint8_t do_read, uint8_t do_write) {
    net_log_pipe_t pipe = ctx;
    net_log_schedule_t schedule = pipe->m_schedule;

    if (do_read) {
        uint8_t need_process = 1;

        while(need_process) {
            ssize_t rv = read(
                pipe->m_pipe_fd[0], pipe->m_pipe_r_buf + pipe->m_pipe_r_size,
                sizeof(pipe->m_pipe_r_buf) - pipe->m_pipe_r_size);
            if (rv < 0) {
                if (errno == EINPROGRESS || errno == EWOULDBLOCK) {
                }
                else {
                    CPE_ERROR(
                        schedule->m_em, "log: pipe %s: read pipe fail, error=%d (%s)!",
                        pipe->m_name, errno, strerror(errno));
                }
                need_process = 0;
            }
            else {
                pipe->m_pipe_r_size += (uint16_t)rv;
                assert(pipe->m_pipe_r_size <= sizeof(pipe->m_pipe_r_buf));
            }

            while(pipe->m_pipe_r_size >= sizeof(struct net_log_pipe_cmd)) {
                net_log_pipe_cmd_t cmd = (net_log_pipe_cmd_t)pipe->m_pipe_r_buf;

                if (pipe->m_pipe_r_size < cmd->m_size) {
                    need_process = 0;
                    break;
                }

                switch(cmd->m_cmd) {
                case net_log_pipe_cmd_send: {
                    net_log_request_param_t send_param = net_log_pipe_pop_send_param(pipe);
                    assert(send_param);

                    if (pipe->m_bind_request_mgr) {
                        net_log_request_manage_process_cmd_send(pipe->m_bind_request_mgr, send_param);
                    }
                    else {
                        CPE_ERROR(schedule->m_em, "log: pipe %s: cmd send: no bind request mgr", pipe->m_name);
                        net_log_category_add_fail_statistics(send_param->category, send_param->log_count);
                        net_log_request_param_free(send_param);
                    }
                    break;
                }
                case net_log_pipe_cmd_pause:
                    if (pipe->m_bind_request_mgr) {
                        net_log_request_manage_process_cmd_pause(pipe->m_bind_request_mgr);
                    }
                    else {
                        CPE_ERROR(schedule->m_em, "log: pipe %s: cmd pause: no bind request mgr", pipe->m_name);
                    }
                    break;
                case net_log_pipe_cmd_resume:
                    if (pipe->m_bind_request_mgr) {
                        net_log_request_manage_process_cmd_resume(pipe->m_bind_request_mgr);
                    }
                    else {
                        CPE_ERROR(schedule->m_em, "log: pipe %s: cmd resume: no bind request mgr", pipe->m_name);
                    }
                    break;
                case net_log_pipe_cmd_stop:
                    if (pipe->m_bind_request_mgr) {
                        net_log_request_manage_process_cmd_stop(pipe->m_bind_request_mgr);
                    }
                    else {
                        CPE_ERROR(schedule->m_em, "log: pipe %s: cmd stop: no bind request mgr", pipe->m_name);
                    }
                    break;
                case net_log_pipe_cmd_stoped:
                    if (schedule->m_main_thread_pipe == pipe) {
                        struct net_log_pipe_cmd_stoped * stoped_cmd = (struct net_log_pipe_cmd_stoped *)cmd;
                        assert(stoped_cmd->head.m_size = sizeof(*stoped_cmd));
                        net_log_schedule_process_cmd_stoped(schedule, stoped_cmd->owner);
                    }
                    else {
                        CPE_ERROR(schedule->m_em, "log: pipe %s: cmd stoped: not in main thread", pipe->m_name);
                    }
                default:
                    CPE_ERROR(schedule->m_em, "log: pipe %s: unknown cmd %d", pipe->m_name, cmd->m_cmd);
                    break;
                }
                
                memmove(pipe->m_pipe_r_buf, pipe->m_pipe_r_buf + cmd->m_size, pipe->m_pipe_r_size - cmd->m_size);
                pipe->m_pipe_r_size -= cmd->m_size;
            }
        }
    }
}
