#ifndef NET_PROVIDER_H_INCLEDED
#define NET_PROVIDER_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

net_router_t
net_router_create(
    net_schedule_t schedule,
    net_address_t address,
    net_protocol_t endpoint_protocol,
    net_driver_t driver);

void net_router_free(net_router_t tunnel_router);

net_address_matcher_t net_router_matcher_white(net_router_t router);
net_address_matcher_t net_router_matcher_white_check_create(net_router_t router);

net_address_matcher_t net_router_matcher_black(net_router_t router);
net_address_matcher_t net_router_matcher_black_check_create(net_router_t router);

NET_END_DECL

#endif
