#ifndef NET_SMUX_PRO_I_H_INCLEDED
#define NET_SMUX_PRO_I_H_INCLEDED
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

CPE_START_PACKED
struct net_smux_head {
    uint8_t m_ver;
    uint8_t m_cmd;
    uint16_t m_len;
    uint32_t m_sid;
} CPE_PACKED;

struct net_smux_body_udp {
    uint32_t m_consumed;
    uint32_t m_window;
} CPE_PACKED;

CPE_END_PACKED

void net_smux_print_frame(write_stream_t ws, struct net_smux_head const * frame);
const char * net_smux_dump_frame(mem_buffer_t buffer, struct net_smux_head const * frame);

#endif
