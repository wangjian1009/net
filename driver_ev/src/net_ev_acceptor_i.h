#ifndef NET_EV_ACCEPTOR_I_H_INCLEDED
#define NET_EV_ACCEPTOR_I_H_INCLEDED
#include "net_ev_driver_i.h"

struct net_ev_acceptor {
    int m_fd;
    struct ev_io m_watcher;
};

int net_ev_acceptor_init(net_acceptor_t base_acceptor);
void net_ev_acceptor_fini(net_acceptor_t base_acceptor);

#endif
