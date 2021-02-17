#include "net_smux_session_i.h"

net_smux_session_t
net_smux_session_create(net_smux_manager_t manager) {
    net_smux_session_t session = mem_alloc(manager->m_alloc, sizeof(struct net_smux_session));

    return session;
}

void net_smux_session_free(net_smux_session_t session) {
}

