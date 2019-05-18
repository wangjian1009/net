#ifndef NET_SOCK_ACCEPTOR_I_H_INCLEDED
#define NET_SOCK_ACCEPTOR_I_H_INCLEDED
#include "net_sock_driver_i.h"

struct net_sock_acceptor {
    int m_fd;
    net_watcher_t m_watcher;
};

int net_sock_acceptor_init(net_acceptor_t base_acceptor);
void net_sock_acceptor_fini(net_acceptor_t base_acceptor);

#endif
