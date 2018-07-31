#ifndef NET_DQ_ACCEPTOR_I_H_INCLEDED
#define NET_DQ_ACCEPTOR_I_H_INCLEDED
#include "net_dq_driver_i.h"

struct net_dq_acceptor {
    int m_fd;
    __unsafe_unretained dispatch_source_t m_source;
};

int net_dq_acceptor_init(net_acceptor_t base_acceptor);
void net_dq_acceptor_fini(net_acceptor_t base_acceptor);

#endif
