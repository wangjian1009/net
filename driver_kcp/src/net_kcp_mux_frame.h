#ifndef NET_KCP_MUX_FRAME_I_H_INCLEDED
#define NET_KCP_MUX_FRAME_I_H_INCLEDED
#include "net_kcp_driver.h"

enum net_kcp_mux_status {
    net_kcp_mux_status_new = 0x01,
    net_kcp_mux_status_keep = 0x02,
    net_kcp_mux_status_end = 0x03,
    net_kcp_mux_status_keep_alive = 0x04,
};

#endif
