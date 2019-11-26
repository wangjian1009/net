#ifndef NET_LIBEVENT_ACCEPTOR_I_H_INCLEDED
#define NET_LIBEVENT_ACCEPTOR_I_H_INCLEDED
#include "net_libevent_driver_i.h"

struct net_libevent_acceptor {
    struct evconnlistener * m_listener;
};

int net_libevent_acceptor_init(net_acceptor_t base_acceptor);
void net_libevent_acceptor_fini(net_acceptor_t base_acceptor);

#endif
