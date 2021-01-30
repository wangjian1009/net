#ifndef NET_WS_SVR_STREAM_DRIVER_I_H_INCLEDED
#define NET_WS_SVR_STREAM_DRIVER_I_H_INCLEDED
#include <wslay/wslay.h>
#include "cpe/utils/error.h"
#include "net_ws_svr_stream_driver.h"

struct net_ws_svr_stream_driver {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    net_driver_t m_underline_driver;
    net_protocol_t m_underline_protocol;
};

mem_buffer_t net_ws_svr_stream_driver_tmp_buffer(net_ws_svr_stream_driver_t driver);

#endif
