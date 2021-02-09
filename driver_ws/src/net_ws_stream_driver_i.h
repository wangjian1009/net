#ifndef NET_WS_STREAM_DRIVER_I_H_INCLEDED
#define NET_WS_STREAM_DRIVER_I_H_INCLEDED
#include "cpe/utils/error.h"
#include "cpe/utils/memory.h"
#include "net_ws_stream_driver.h"

struct net_ws_stream_driver {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    net_driver_t m_underline_driver;
    net_protocol_t m_underline_protocol;
};

mem_buffer_t net_ws_stream_driver_tmp_buffer(net_ws_stream_driver_t driver);

#endif
