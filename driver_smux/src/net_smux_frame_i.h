#ifndef NET_SMUX_FRAME_I_H_INCLEDED
#define NET_SMUX_FRAME_I_H_INCLEDED
#include "cpe/pal/pal_platform.h"
#include "net_smux_protocol_i.h"

enum net_smux_cmd {
	/* protocol version 1 */
    net_smux_cmd_syn = 0x01,
    net_smux_cmd_fin = 0x02,
    net_smux_cmd_psh = 0x03,
    net_smux_cmd_nop = 0x04,
	/* protocol version 2 */
    net_smux_cmd_udp = 0x05,
};

enum net_smux_status {
    net_smux_status_new = 0x01,
    net_smux_status_keep = 0x02,
    net_smux_status_end = 0x03,
    net_smux_status_keep_alive = 0x04,
};

CPE_START_PACKED
struct net_smux_frame_head {
    uint8_t m_ver;
    uint8_t m_cmd;
    uint16_t m_len;
    uint32_t m_sid;
} CPE_PACKED;
CPE_END_PACKED

struct net_smux_frame {
    uint16_t m_capacity;
    union {
        TAILQ_ENTRY(net_smux_frame) m_next;
        struct net_smux_frame_head m_head;
    };
};

net_smux_frame_t net_smux_frame_create(
    net_smux_session_t session, uint8_t cmd, uint32_t sid, uint16_t len);

void net_smux_frame_free(net_smux_session_t session, net_smux_frame_t frame);

#endif
