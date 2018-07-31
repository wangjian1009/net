#ifndef NET_ACCEPTOR_I_H_INCLEDED
#define NET_ACCEPTOR_I_H_INCLEDED
#include "net_acceptor.h"
#include "net_driver_i.h"

NET_BEGIN_DECL

struct net_acceptor {
    net_driver_t m_driver;
    TAILQ_ENTRY(net_acceptor) m_next_for_driver;
    net_address_t m_address;
    net_protocol_t m_protocol;
    uint32_t m_queue_size;
};

void net_acceptor_real_free(net_acceptor_t acceptor);

NET_END_DECL

#endif
