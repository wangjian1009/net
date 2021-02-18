#ifndef NET_SMUX_FRAME_I_H_INCLEDED
#define NET_SMUX_FRAME_I_H_INCLEDED
#include "cpe/pal/pal_platform.h"
#include "net_smux_protocol_i.h"

enum net_smux_status {
    net_smux_status_new = 0x01,
    net_smux_status_keep = 0x02,
    net_smux_status_end = 0x03,
    net_smux_status_keep_alive = 0x04,
};

struct net_smux_frame {
    union {
        TAILQ_ENTRY(net_smux_frame) m_next;
        struct {
            uint8_t m_cmd;
            uint16_t m_len;
            uint32_t m_sid;
            void * m_data;
        };
    };
};

net_smux_frame_t
net_smux_frame_create(
    net_smux_session_t session, net_smux_stream_t stream, uint8_t cmd, uint16_t len);

void net_smux_frame_free(
    net_smux_session_t session, net_smux_stream_t stream, net_smux_frame_t frame);

void net_smux_frame_real_free(net_smux_protocol_t protocol, net_smux_frame_t frame);

#endif
