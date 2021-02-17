#ifndef NET_KCP_SMUX_FRAME_I_H_INCLEDED
#define NET_KCP_SMUX_FRAME_I_H_INCLEDED
#include "cpe/pal/pal_platform.h"
#include "net_kcp_driver.h"

enum net_kcp_smux_cmd {
	/* protocol version 1 */
    net_kcp_smux_cmd_syn = 0x01,
    net_kcp_smux_cmd_fin = 0x02,
    net_kcp_smux_cmd_psh = 0x03,
    net_kcp_smux_cmd_nop = 0x04,
	/* protocol version 2 */
    net_kcp_smux_cmd_udp = 0x05,
};

enum net_kcp_smux_status {
    net_kcp_smux_status_new = 0x01,
    net_kcp_smux_status_keep = 0x02,
    net_kcp_smux_status_end = 0x03,
    net_kcp_smux_status_keep_alive = 0x04,
};

CPE_START_PACKED

struct net_pkc_mux_frame {
    uint8_t m_ver;
    uint8_t m_cmd;
    uint32_t m_sid;
    uint8_t m_data[];
} CPE_PACKED;

CPE_END_PACKED

#endif
