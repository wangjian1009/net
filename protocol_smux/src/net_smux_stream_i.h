#ifndef NET_SMUX_STREAM_I_H_INCLEDED
#define NET_SMUX_STREAM_I_H_INCLEDED
#include "net_smux_stream.h"
#include "net_smux_session_i.h"

NET_BEGIN_DECL

struct net_smux_stream {
    net_smux_session_t m_session;
};
    
NET_END_DECL

#endif
