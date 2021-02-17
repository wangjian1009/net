#ifndef NET_SMUX_SESSION_I_H_INCLEDED
#define NET_SMUX_SESSION_I_H_INCLEDED
#include "cpe/utils/hash.h"
#include "net_smux_session.h"
#include "net_smux_manager_i.h"

NET_BEGIN_DECL

struct net_smux_session {
    net_smux_manager_t m_manager;
    TAILQ_ENTRY(net_smux_session) m_next;
    uint32_t m_session_id;
    net_smux_session_runing_mode_t m_runing_mode;

    uint32_t m_max_stream_id; /* next stream identifier */
    struct cpe_hash_table m_streams;

    struct {
        net_smux_session_underline_type_t m_type;
        union {
            struct {
                net_endpoint_t m_endpoint;
            } m_tcp;
            struct {
                net_dgram_t m_dgram;
            } m_udp;
        };
    } m_underline;

    net_timer_t m_timer_ping;
    net_timer_t m_timer_timeout;
};

int net_smux_session_write_frame(
    net_smux_session_t session, net_smux_cmd_t cmd, uint32_t sid);

NET_END_DECL

#endif
