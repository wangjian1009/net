#ifndef NET_NE_UTILS_H_INCLEDED
#define NET_NE_UTILS_H_INCLEDED
#import <NetworkExtension/NWHostEndpoint.h>
#include "net_ne_driver_i.h"

NWHostEndpoint * net_ne_address_to_endoint(net_ne_driver_t driver, net_address_t address);

#endif
