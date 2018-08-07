#include "net_address.h"
#include "net_driver.h"
#include "net_ne_utils.h"

NWHostEndpoint * net_ne_address_to_endoint(net_ne_driver_t driver, net_address_t address) {
    char port[16];
    snprintf(port, sizeof(port), "%d", net_address_port(address));
    const char * host = net_address_host(net_ne_driver_tmp_buffer(driver), address);
    
    return [NWHostEndpoint endpointWithHostname: [[NSString alloc] initWithCString:host encoding:NSUTF8StringEncoding]
                                   port: [[NSString alloc] initWithCString:port encoding:NSUTF8StringEncoding]
            ];
}

net_address_t net_ne_endpoint_to_address(net_ne_driver_t driver, NWEndpoint * endpoint) {
    NWHostEndpoint * hostEndpoint = (NWHostEndpoint * )endpoint;
    if (hostEndpoint == nil) {
        CPE_ERROR(driver->m_em, "net_ne_endpoint_to_address: not NWHostEndpoint!");
        return NULL;
    }

    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));
    net_address_t address = net_address_create_auto(schedule, [[hostEndpoint hostname] UTF8String]);
    if (address == NULL) {
        CPE_ERROR(driver->m_em, "net_ne_endpoint_to_address: create from %s fail!", [[hostEndpoint hostname] UTF8String]);
        return NULL;
    }

    net_address_set_port(address, atoi([[hostEndpoint port] UTF8String]));

    return address;
}
