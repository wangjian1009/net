#ifndef NET_ROUTER_H_INCLEDED
#define NET_ROUTER_H_INCLEDED
#include "net_router_types.h"

NET_BEGIN_DECL

net_router_t
net_router_create(
    net_router_schedule_t schedule,
    net_address_t address,
    net_protocol_t endpoint_protocol,
    net_driver_t driver,
    net_acceptor_on_new_endpoint_fun_t on_new_endpoint, void * on_new_endpoint_ctx);

void net_router_free(net_router_t tunnel_router);

net_address_t net_router_address(net_router_t router);
net_protocol_t net_router_protocol(net_router_t router);
net_driver_t net_router_driver(net_router_t router);

net_address_matcher_t net_router_matcher_white(net_router_t router);
net_address_matcher_t net_router_matcher_white_check_create(net_router_t router);

net_address_matcher_t net_router_matcher_black(net_router_t router);
net_address_matcher_t net_router_matcher_black_check_create(net_router_t router);

int net_router_link(net_router_t router, net_endpoint_t endpoint, net_address_t target_addr, uint8_t is_own);

NET_END_DECL

#endif
