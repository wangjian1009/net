#include "net_address.h"
#include "net_ne_utils.h"

NWHostEndpoint * net_ne_address_to_endoint(net_ne_driver_t driver, net_address_t address) {
    char port[16];
    snprintf(port, sizeof(port), "%d", net_address_port(address));
    const char * host = net_address_host(net_ne_driver_tmp_buffer(driver), address);
    
    return [NWHostEndpoint endpointWithHostname: [[NSString alloc] initWithCString:host encoding:NSUTF8StringEncoding]
                                   port: [[NSString alloc] initWithCString:port encoding:NSUTF8StringEncoding]
            ];
}
