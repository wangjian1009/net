#ifndef NET_SMUX_STREAM_H_INCLEDED
#define NET_SMUX_STREAM_H_INCLEDED
#include "net_smux_types.h"

NET_BEGIN_DECL

net_smux_stream_t net_smux_stream_create(net_smux_session_t session);
void net_smux_stream_free(net_smux_stream_t stream);

NET_END_DECL

#endif
