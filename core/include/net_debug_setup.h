#ifndef NET_DEBUG_SETUP_H_INCLEDED
#define NET_DEBUG_SETUP_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

net_debug_setup_t
net_debug_setup_create(net_schedule_t schedule, uint8_t debug_protocol, uint8_t debug_driver);

void net_debug_setup_free(net_debug_setup_t debug_setup);

NET_END_DECL

#endif
