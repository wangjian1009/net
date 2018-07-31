#ifndef NET_DQ_ACCEPTOR_I_H_INCLEDED
#define NET_DQ_ACCEPTOR_I_H_INCLEDED
#include "net_dq_acceptor.h"
#include "net_dq_driver_i.h"

struct net_dq_acceptor {
    net_dq_driver_t m_driver;
    net_address_t m_address;
    TAILQ_ENTRY(net_dq_acceptor) m_next_for_driver;
    net_protocol_t m_protocol;
    int m_fd;
    __unsafe_unretained dispatch_source_t m_source;
};

#endif
