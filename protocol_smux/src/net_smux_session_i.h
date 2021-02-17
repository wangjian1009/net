#ifndef NET_SMUX_SESSION_I_H_INCLEDED
#define NET_SMUX_SESSION_I_H_INCLEDED
#include "cpe/utils/hash.h"
#include "net_smux_session.h"
#include "net_smux_manager_i.h"

NET_BEGIN_DECL

struct net_smux_session {
    net_smux_manager_t m_manager;
    TAILQ_ENTRY(net_smux_session) m_next;
    net_smux_session_runing_mode_t m_runing_mode;

    uint32_t m_next_stream_id; /* next stream identifier */
    struct cpe_hash_table m_streams;
};

NET_END_DECL

#endif
