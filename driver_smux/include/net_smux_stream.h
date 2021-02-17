#ifndef NET_SMUX_STREAM_H_INCLEDED
#define NET_SMUX_STREAM_H_INCLEDED
#include "net_smux_types.h"

NET_BEGIN_DECL

enum net_smux_stream_state {
    net_smux_stream_state_init,
};

net_smux_stream_t net_smux_stream_create(net_smux_session_t session);
void net_smux_stream_free(net_smux_stream_t stream);

uint32_t net_smux_stream_id(net_smux_stream_t stream);
net_smux_session_t net_smux_stream_session(net_smux_stream_t stream);
net_smux_stream_state_t net_smux_stream_state(net_smux_stream_t stream);

NET_END_DECL

#endif
