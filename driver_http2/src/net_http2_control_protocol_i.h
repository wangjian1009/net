#ifndef NET_HTTP2_CONTROL_PROTOCOL_I_H_INCLEDED
#define NET_HTTP2_CONTROL_PROTOCOL_I_H_INCLEDED
#include "cpe/utils/error.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/buffer.h"
#include "net_http2_control_protocol.h"

struct net_http2_control_protocol {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    struct mem_buffer m_data_buffer;
};

mem_buffer_t net_http2_control_protocol_tmp_buffer(net_http2_control_protocol_t protocol);

#endif
