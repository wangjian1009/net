#ifndef NET_HTTP2_STREAM_DRIVER_I_H_INCLEDED
#define NET_HTTP2_STREAM_DRIVER_I_H_INCLEDED
#include "cpe/utils/error.h"
#include "cpe/utils/memory.h"
#include "net_http2_driver.h"

struct net_http2_driver {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    net_driver_t m_control_driver;
    net_protocol_t m_control_protocol;
};

mem_buffer_t net_http2_driver_tmp_buffer(net_http2_driver_t driver);

#endif
