#ifndef NET_NE_ENDPOINT_H_INCLEDED
#define NET_NE_ENDPOINT_H_INCLEDED
#import <NetworkExtension/NWTCPConnection.h>
#include "net_ne_driver_i.h"

@interface NetNeEndpointObserver : NSObject {
}
@end

struct net_ne_endpoint {
    __unsafe_unretained NWTCPConnection * m_connection;
    __unsafe_unretained NetNeEndpointObserver * m_observer;
    uint8_t m_is_writing;
};

int net_ne_endpoint_init(net_endpoint_t base_endpoint);
void net_ne_endpoint_fini(net_endpoint_t base_endpoint);
int net_ne_endpoint_connect(net_endpoint_t base_endpoint);
void net_ne_endpoint_close(net_endpoint_t base_endpoint);
int net_ne_endpoint_on_output(net_endpoint_t base_endpoint);

void net_ne_endpoint_start_rw_watcher(
    net_ne_driver_t driver, net_endpoint_t base_endpoint, net_ne_endpoint_t endpoint);
int net_ne_endpoint_update_local_address(net_ne_endpoint_t endpoint);
int net_ne_endpoint_update_remote_address(net_ne_endpoint_t endpoint);

#endif
