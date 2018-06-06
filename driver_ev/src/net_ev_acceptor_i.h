#ifndef NET_EV_ACCEPTOR_I_H_INCLEDED
#define NET_EV_ACCEPTOR_I_H_INCLEDED
#include "net_ev_acceptor.h"
#include "net_ev_driver_i.h"

struct net_ev_acceptor {
    net_ev_driver_t m_driver;
    net_address_t m_address;
    TAILQ_ENTRY(net_ev_acceptor) m_next_for_driver;
    net_protocol_t m_protocol;
    int m_fd;
    struct ev_io m_watcher;
};

#endif
