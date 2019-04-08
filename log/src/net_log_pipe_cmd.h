#ifndef NET_LOG_PIPE_CMD_I_H_INCLEDED
#define NET_LOG_PIPE_CMD_I_H_INCLEDED
#include "net_log_schedule_i.h"

NET_BEGIN_DECL

typedef enum net_log_pipe_cmd_type {
    net_log_pipe_cmd_send,
    net_log_pipe_cmd_pause,
    net_log_pipe_cmd_resume,
    net_log_pipe_cmd_stop_begin,
    net_log_pipe_cmd_stop_complete,
    net_log_pipe_cmd_stoped,
} net_log_pipe_cmd_type_t;

struct net_log_pipe_cmd {
    uint8_t m_size;
    uint8_t m_cmd;
};

struct net_log_pipe_cmd_stoped {
    struct net_log_pipe_cmd head;
    void * owner;
};

NET_END_DECL

#endif

