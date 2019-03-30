#ifndef NET_LOG_PIPE_CMD_I_H_INCLEDED
#define NET_LOG_PIPE_CMD_I_H_INCLEDED
#include "net_log_schedule_i.h"

NET_BEGIN_DECL

typedef enum net_log_pipe_cmd_type {
    net_log_pipe_cmd_send,
} net_log_pipe_cmd_type_t;

struct net_log_pipe_cmd {
    uint8_t m_size;
    net_log_pipe_cmd_type_t m_type;
};

NET_END_DECL

#endif

