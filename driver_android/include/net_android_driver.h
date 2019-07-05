#ifndef NET_ANDROID_DRIVER_H_INCLEDED
#define NET_ANDROID_DRIVER_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_android_types.h"

NET_BEGIN_DECL

struct ev_loop;
net_android_driver_t net_android_driver_create(net_schedule_t schedule, struct ev_loop * ev_loop);

void net_android_driver_free(net_android_driver_t driver);

net_driver_t net_android_driver_base_driver(net_android_driver_t driver);

NET_END_DECL

#endif
