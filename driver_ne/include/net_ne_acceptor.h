#ifndef NET_NE_ACCEPTOR_H_INCLEDED
#define NET_NE_ACCEPTOR_H_INCLEDED
#include "net_ne_types.h"

NET_BEGIN_DECL

net_ne_acceptor_t
net_ne_acceptor_create(
    net_ne_driver_t driver, net_protocol_t protocol,
    net_address_t address, uint32_t accept_queue_size);

void net_ne_acceptor_free(
    net_ne_acceptor_t acceptor);

NET_END_DECL

#endif
