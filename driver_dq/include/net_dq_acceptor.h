#ifndef NET_DQ_ACCEPTOR_H_INCLEDED
#define NET_DQ_ACCEPTOR_H_INCLEDED
#include "net_dq_types.h"

NET_BEGIN_DECL

net_dq_acceptor_t
net_dq_acceptor_create(
    net_dq_driver_t driver, net_protocol_t protocol,
    net_address_t address, uint32_t accept_queue_size);

void net_dq_acceptor_free(
    net_dq_acceptor_t acceptor);

NET_END_DECL

#endif
