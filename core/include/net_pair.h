#ifndef NET_CORE_PAIR_H_INCLEDED
#define NET_CORE_PAIR_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

/*driver*/
net_driver_t net_schedule_pair_driver(net_schedule_t schedule);

/*endpoint*/
int net_pair_endpoint_make(
    net_schedule_t schedule,
    net_protocol_t p0, net_protocol_t p1,
    net_endpoint_t ep[2]);

net_endpoint_t net_pair_endpoint_other(net_endpoint_t endpoint);

int net_pair_endpoint_link(net_endpoint_t a, net_endpoint_t z);

NET_END_DECL

#endif
