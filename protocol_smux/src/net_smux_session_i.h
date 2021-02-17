#ifndef NET_SMUX_SESSION_I_H_INCLEDED
#define NET_SMUX_SESSION_I_H_INCLEDED
#include "net_smux_session.h"
#include "net_smux_manager_i.h"

NET_BEGIN_DECL

struct net_smux_session {
    net_smux_manager_t m_manager;
};
    
NET_END_DECL

#endif
