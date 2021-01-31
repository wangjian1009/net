#ifndef NET_WS_CLI_PROTOCOL_I_H_INCLEDED
#define NET_WS_CLI_PROTOCOL_I_H_INCLEDED
#include "cpe/utils/error.h"
#include "cpe/utils/memory.h"
#include "net_ws_protocol.h"

struct net_ws_protocol {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
};

mem_buffer_t net_ws_protocol_tmp_buffer(net_ws_protocol_t protocol);

#endif
