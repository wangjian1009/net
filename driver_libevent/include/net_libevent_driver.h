#ifndef NET_LIBEVENT_DRIVER_H_INCLEDED
#define NET_LIBEVENT_DRIVER_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_libevent_types.h"

NET_BEGIN_DECL

net_libevent_driver_t net_libevent_driver_create(net_schedule_t schedule, struct event_base * event_base);

void net_libevent_driver_free(net_libevent_driver_t driver);

net_libevent_driver_t net_libevent_driver_cast(net_driver_t base_driver);
net_driver_t net_libevent_driver_base_driver(net_libevent_driver_t driver);
struct event_base * net_libevent_driver_event_base(net_libevent_driver_t driver);

NET_END_DECL

#endif
