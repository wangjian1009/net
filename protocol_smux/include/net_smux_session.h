#ifndef NET_SMUX_SESSION_H_INCLEDED
#define NET_SMUX_SESSION_H_INCLEDED
#include "net_smux_types.h"

NET_BEGIN_DECL

net_smux_session_t net_smux_session_create(net_smux_manager_t manager);
void net_smux_session_free(net_smux_session_t session);

NET_END_DECL

#endif
