#ifndef NET_ACCEPTOR_H_INCLEDED
#define NET_ACCEPTOR_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

net_acceptor_t
net_acceptor_create(
    net_driver_t driver, net_protocol_t protocol,
    net_address_t address, uint8_t is_own, uint32_t accept_queue_size,
    net_acceptor_on_new_endpoint_fun_t on_new_endpoint, void * on_new_endpoint_ctx);

void net_acceptor_free(net_acceptor_t acceptor);

net_schedule_t net_acceptor_schedule(net_acceptor_t acceptor);
net_driver_t net_acceptor_driver(net_acceptor_t acceptor);
net_protocol_t net_acceptor_protocol(net_acceptor_t acceptor);
net_address_t net_acceptor_address(net_acceptor_t acceptor);
uint32_t net_acceptor_queue_size(net_acceptor_t acceptor);

int net_acceptor_on_new_endpoint(net_acceptor_t acceptor, net_endpoint_t endpoint);

void * net_acceptor_data(net_acceptor_t acceptor);
net_acceptor_t net_acceptor_from_data(void * data);

NET_END_DECL

#endif
