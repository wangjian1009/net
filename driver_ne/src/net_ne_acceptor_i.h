#ifndef NET_NE_ACCEPTOR_I_H_INCLEDED
#define NET_NE_ACCEPTOR_I_H_INCLEDED
#include "net_ne_acceptor.h"
#include "net_ne_driver_i.h"

struct net_ne_acceptor {
    net_ne_driver_t m_driver;
    net_address_t m_address;
    TAILQ_ENTRY(net_ne_acceptor) m_next_for_driver;
    net_protocol_t m_protocol;
};

#endif
