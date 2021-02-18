#include "net_smux_frame_i.h"
#include "net_smux_session_i.h"

net_smux_frame_t
net_smux_frame_create(
    net_smux_session_t session, uint8_t cmd, uint32_t sid, uint16_t len)
{
    net_smux_protocol_t protocol = session->m_protocol;

    net_smux_frame_t frame = TAILQ_FIRST(&protocol->m_free_frames);
    if (frame) {
        TAILQ_REMOVE(&protocol->m_free_frames, frame, m_next);
    }
    else {
        frame = mem_alloc(protocol->m_alloc, sizeof(struct net_smux_frame));
        if (frame == NULL) {
            CPE_ERROR(
                protocol->m_em, "smux: %s: frame: create: alloc fail",
                net_smux_session_dump(net_smux_protocol_tmp_buffer(protocol), session));
            return NULL;
        }
    }

    return frame;
}

void net_smux_frame_free(net_smux_session_t session, net_smux_frame_t frame) {
    net_smux_protocol_t protocol = session->m_protocol;

    
    TAILQ_INSERT_TAIL(&protocol->m_free_frames, frame, m_next);
}

void net_smux_frame_real_free(net_smux_protocol_t protocol, net_smux_frame_t frame) {
    TAILQ_REMOVE(&protocol->m_free_frames, frame, m_next);
    mem_free(protocol->m_alloc, frame);
}

