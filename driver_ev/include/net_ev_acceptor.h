#ifndef NET_EV_ACCEPTOR_H_INCLEDED
#define NET_EV_ACCEPTOR_H_INCLEDED
#include "net_ev_types.h"

NET_BEGIN_DECL

net_ev_acceptor_t
net_ev_acceptor_create(
    net_ev_driver_t driver, net_protocol_t protocol,
    net_address_t address, uint32_t accept_queue_size);

void net_ev_acceptor_free(
    net_ev_acceptor_t acceptor);

NET_END_DECL

#endif
