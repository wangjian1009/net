#ifndef NET_SMUX_STREAM_I_H_INCLEDED
#define NET_SMUX_STREAM_I_H_INCLEDED
#include "net_smux_stream.h"
#include "net_smux_session_i.h"

NET_BEGIN_DECL

struct net_smux_stream {
    net_smux_session_t m_session;
    struct cpe_hash_entry m_hh_for_session;
    uint32_t m_stream_id;
    net_smux_stream_state_t m_state;
};

net_smux_stream_t
net_smux_stream_create(
    net_smux_session_t session, uint32_t stream_id);

void net_smux_stream_free_all(net_smux_session_t session);

int net_smux_stream_eq(net_smux_stream_t l, net_smux_stream_t r, void * user_data);
uint32_t net_smux_stream_hash(net_smux_stream_t o, void * user_data);

NET_END_DECL

#endif
