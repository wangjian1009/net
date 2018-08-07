#include "assert.h"
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/string_utils.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_driver.h"
#include "net_ne_endpoint.h"
#include "net_ne_utils.h"

static const char * net_ne_endpoint_state_str(NWTCPConnectionState state);
static int net_ne_endpoint_do_output(net_ne_driver_t driver, net_ne_endpoint_t endpoint, net_endpoint_t base_endpoint);
static void net_ne_endpoint_do_read(net_ne_driver_t driver, net_ne_endpoint_t endpoint, net_endpoint_t base_endpoint);

int net_ne_endpoint_init(net_endpoint_t base_endpoint) {
    net_ne_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_ne_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    endpoint->m_connection = nil;
    endpoint->m_observer = [[[NetNeEndpointObserver alloc] init] retain];
    endpoint->m_is_writing = 0;
    
    CPE_ERROR(
        driver->m_em, "ne: %s: init",
        net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint));

    return 0;
}

void net_ne_endpoint_fini(net_endpoint_t base_endpoint) {
    net_ne_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_ne_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    CPE_ERROR(
        driver->m_em, "ne: %s: fini",
        net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint));
    
    if (endpoint->m_connection) {
        [endpoint->m_connection cancel];

        CPE_ERROR(
            driver->m_em, "ne: %s: fini, is-writing=%d",
            net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint), endpoint->m_is_writing);

        [endpoint->m_connection release];
        endpoint->m_connection = nil;
    }

    [endpoint->m_observer release];
    endpoint->m_observer = nil;
}

int net_ne_endpoint_on_output(net_endpoint_t base_endpoint) {
    @autoreleasepool {
        net_ne_endpoint_t endpoint = net_endpoint_data(base_endpoint);
        net_ne_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
        return net_ne_endpoint_do_output(driver, endpoint, base_endpoint);
    }
}

int net_ne_endpoint_connect(net_endpoint_t base_endpoint) {
    net_ne_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_ne_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);

    if (endpoint->m_connection != nil) {
        CPE_ERROR(
            driver->m_em, "ne: %s: already connected!",
            net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint));
        return -1;
    }

    @autoreleasepool {
        net_address_t remote_addr = net_endpoint_remote_address(base_endpoint);
        if (remote_addr == NULL) {
            CPE_ERROR(
                driver->m_em, "ne: %s: connect with no remote address!",
                net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint));
            return -1;
        }

        NWHostEndpoint * remote_endpoint = net_ne_address_to_endoint(driver, remote_addr);
        if (remote_endpoint == NULL) {
            CPE_ERROR(
                driver->m_em, "ne: %s: convert remote address fail!",
                net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint));
            return -1;
        }
    
        endpoint->m_connection = [driver->m_tunnel_provider createTCPConnectionToEndpoint: remote_endpoint
                                                                                enableTLS: false
                                                                            TLSParameters: nil
                                                                                 delegate: nil];
        if (endpoint->m_connection == nil) {
            CPE_ERROR(
                driver->m_em, "ne: %s: create connection fail!",
                net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint));
            return -1;
        }
        [endpoint->m_connection retain];

        [endpoint->m_connection addObserver: endpoint->m_observer
                                 forKeyPath: @"state" 
                                    options: NSKeyValueObservingOptionOld | NSKeyValueObservingOptionNew
                                    context: endpoint];

        switch(endpoint->m_connection.state) {
        case NWTCPConnectionStateConnecting:
            if (driver->m_debug || net_schedule_debug(schedule) >= 2) {
                CPE_INFO(
                    driver->m_em, "ne: %s: connect start",
                    net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint));
            }

            return net_endpoint_set_state(base_endpoint, net_endpoint_state_connecting);
        case NWTCPConnectionStateConnected:
            if (driver->m_debug || net_schedule_debug(schedule) >= 2) {
                CPE_INFO(
                    driver->m_em, "ne: %s: connect success",
                    net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint));
            }

            if (net_endpoint_address(base_endpoint) == NULL) {
                net_ne_endpoint_update_local_address(endpoint);
            }

            return net_endpoint_set_state(base_endpoint, net_endpoint_state_established);
        case NWTCPConnectionStateCancelled:
        case NWTCPConnectionStateInvalid:
        case NWTCPConnectionStateDisconnected:
        default:
            CPE_ERROR(
                driver->m_em, "ne: %s: connect complete, state=%s, error",
                net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint),
                net_ne_endpoint_state_str(endpoint->m_connection.state));
            [endpoint->m_connection release];
            endpoint->m_connection = nil;
            return -1;
        }
    }
}

void net_ne_endpoint_close(net_endpoint_t base_endpoint) {
    net_ne_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (endpoint->m_connection == nil) return;

    [endpoint->m_connection cancel];
    [endpoint->m_connection release];
    endpoint->m_connection = nil;
}

int net_ne_endpoint_update_local_address(net_ne_endpoint_t endpoint) {
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_ne_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    @autoreleasepool {
        NWEndpoint *localAddress = [endpoint->m_connection localAddress];
        if (localAddress == nil) {
            CPE_ERROR(
                driver->m_em, "ne: %s: localAddress error",
                net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint));
            return -1;
        }

        net_address_t address = net_ne_endpoint_to_address(driver, localAddress);
        if (address == NULL) {
            CPE_ERROR(
                driver->m_em, "ne: %s: create local address fail",
                net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint));
            return -1;
        }

        net_endpoint_set_address(base_endpoint, address, 1);

        return 0;
    }
}

int net_ne_endpoint_update_remote_address(net_ne_endpoint_t endpoint) {
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_ne_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    
    @autoreleasepool {
        NWEndpoint * remoteAddress = [endpoint->m_connection remoteAddress];
        if (remoteAddress == nil) {
            CPE_ERROR(
                driver->m_em, "ne: %s: address error",
                net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint));
            return -1;
        }

        net_address_t address = net_ne_endpoint_to_address(driver, remoteAddress);
        if (address == NULL) {
            CPE_ERROR(
                driver->m_em, "ne: %s: create remote address fail",
                net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint));
            return -1;
        }

        net_endpoint_set_remote_address(base_endpoint, address, 1);
    
        return 0;
    }
}

static const char * net_ne_endpoint_state_str(NWTCPConnectionState state) {
    switch(state) {
    case NWTCPConnectionStateConnecting:
        return "Connecting";
    case NWTCPConnectionStateConnected:
        return "Connected";
    case NWTCPConnectionStateCancelled:
        return "Cancelled";
    case NWTCPConnectionStateInvalid:
        return "Invalid";
    case NWTCPConnectionStateDisconnected:
        return "Disconnected";
    case NWTCPConnectionStateWaiting:
        return "Waiting";
    default:
        return "Unknown";
    }
}

static void net_ne_endpoint_do_read(net_ne_driver_t driver, net_ne_endpoint_t endpoint, net_endpoint_t base_endpoint) {
    [endpoint->m_connection
        readMinimumLength: 1
        maximumLength: 255
        completionHandler: ^(NSData *data, NSError *error) {
            dispatch_sync(
                dispatch_get_main_queue(),
                ^{
                    if (error) {
                        CPE_ERROR(
                            driver->m_em, "ne: %s: recv error, %s",
                            net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint),
                            [[error localizedDescription] UTF8String]);
                
                        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
                            net_endpoint_free(base_endpoint);
                            return;
                        }
                        return;
                    }

                    uint32_t bytes = (uint32_t)data.length;
                    void * buf = net_endpoint_rbuf_alloc(base_endpoint, &bytes);
                    if (buf == NULL || bytes != (uint32_t)data.length) {
                        CPE_ERROR(
                            driver->m_em, "ne: %s: recv %d bytes data, alloc endpoint rbuf fail, r-size=%d!",
                            net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint),
                            (uint32_t)data.length, bytes);
                
                        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
                            net_endpoint_free(base_endpoint);
                            return;
                        }
                    }

                    [data getBytes: buf length: bytes];
                    if (driver->m_debug) {
                        CPE_INFO(
                            driver->m_em, "ne: %s: recv %d bytes data!",
                            net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint),
                            bytes);
                    }

                    if (net_endpoint_rbuf_supply(base_endpoint, bytes) != 0) {
                        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_logic_error) != 0) {
                            if (driver->m_debug) {
                                CPE_INFO(
                                    driver->m_em, "ne: %s: free for process fail!",
                                    net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint));
                            }
                            net_endpoint_free(base_endpoint);
                        }
                        return;
                    }

                    if (driver->m_data_monitor_fun) {
                        driver->m_data_monitor_fun(driver->m_data_monitor_ctx, base_endpoint, net_data_in, (uint32_t)bytes);
                    }

                    net_ne_endpoint_do_read(driver, endpoint, base_endpoint);
                });
        }];
}

static int net_ne_endpoint_do_output(net_ne_driver_t driver, net_ne_endpoint_t endpoint, net_endpoint_t base_endpoint) {
    @autoreleasepool {
        if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return 0;
        if (endpoint->m_is_writing) return 0;

        uint32_t sz;
        void * buf = net_endpoint_wbuf(base_endpoint, &sz);
        if (buf == NULL) return 0;

        NSData * data = [NSData dataWithBytes: buf length: sz];
        if (data == nil) {
            CPE_ERROR(
                driver->m_em, "ne: %s: output: copy data fail",
                net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint));
            return -1;
        }
        net_endpoint_wbuf_consume(base_endpoint, sz);

        if (driver->m_data_monitor_fun) {
            driver->m_data_monitor_fun(driver->m_data_monitor_ctx, base_endpoint, net_data_out, sz);
        }
    
        endpoint->m_is_writing = 1;
        [endpoint->m_connection
            write: data
            completionHandler: ^(NSError* error) {
                dispatch_sync(
                    dispatch_get_main_queue(),
                    ^{
                        endpoint->m_is_writing = 0;

                        if (error != nil) {
                            CPE_ERROR(
                                driver->m_em, "ne: %s: disconnected in connecting, connect error!, %s",
                                net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint),
                                error ? [[error localizedDescription] UTF8String] : "unknown error");
                
                            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
                                net_endpoint_free(base_endpoint);
                                return;
                            }

                            return;
                        }

                        if (net_ne_endpoint_do_output(driver, endpoint, base_endpoint) != 0) {
                            CPE_ERROR(
                                driver->m_em, "ne: %s: auto send follow data error!",
                                net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint));
                            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
                                net_endpoint_free(base_endpoint);
                                return;
                            }
                        }
                    });
            }];

        if (driver->m_debug) {
            CPE_INFO(
                driver->m_em, "ne: %s: send %d bytes data!",
                net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint),
                sz);
        }
    }

    return 0;
}

@implementation NetNeEndpointObserver
-(void)observeValueForKeyPath:(NSString *)keyPath 
                     ofObject:(id)object 
                       change:(NSDictionary *)change 
                      context:(void *)context
{
    dispatch_sync(
        dispatch_get_main_queue(),
        ^{
            @autoreleasepool {
                net_ne_endpoint_t endpoint = context;
                net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
                net_ne_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
                net_schedule_t schedule = net_endpoint_schedule(base_endpoint);

                if ([keyPath isEqual:@"state"]) {
                    NWTCPConnectionState oldState = (NWTCPConnectionState)[[change objectForKey:@"old"] intValue];
                    NWTCPConnectionState newState = (NWTCPConnectionState)[[change objectForKey:@"new"] intValue];
        
                    CPE_INFO(
                        driver->m_em, "ne: %s: state %s(%d) ==> %s(%d)!",
                        net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint),
                        net_ne_endpoint_state_str(oldState), (int)oldState, net_ne_endpoint_state_str(newState), (int)newState);

                    switch(newState) {
                    case NWTCPConnectionStateWaiting:
                    case NWTCPConnectionStateDisconnected:
                        if (net_endpoint_state(base_endpoint) == net_endpoint_state_connecting) {
                            NSError * error = [endpoint->m_connection error];
                            CPE_ERROR(
                                driver->m_em, "ne: %s: disconnected in connecting, connect error!, %s",
                                net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint),
                                error ? [[error localizedDescription] UTF8String] : "unknown error");
                
                            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
                                net_endpoint_free(base_endpoint);
                                return;
                            }
                        }
                        else if (net_endpoint_state(base_endpoint) == net_endpoint_state_established) {
                            if (driver->m_debug || net_schedule_debug(schedule) >= 2) {
                                NSError * error = [endpoint->m_connection error];
                                CPE_INFO(
                                    driver->m_em, "ne: %s: disconnected!, %s",
                                    net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint),
                                    error ? [[error localizedDescription] UTF8String] : "unknown error");
                            }

                            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_disable) != 0) {
                                net_endpoint_free(base_endpoint);
                                return;
                            }
                        }
                        else {
                            NSError * error = [endpoint->m_connection error];
                            CPE_ERROR(
                                driver->m_em, "ne: %s: disconnected in state %s error!, %s",
                                net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint),
                                net_endpoint_state_str(net_endpoint_state(base_endpoint)),
                                error ? [[error localizedDescription] UTF8String] : "unknown error");

                            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
                                net_endpoint_free(base_endpoint);
                                return;
                            }
                        }
                        break;
                    case NWTCPConnectionStateConnected:
                        if (net_endpoint_state(base_endpoint) == net_endpoint_state_connecting) {
                            if (net_endpoint_address(base_endpoint) == NULL) {
                                net_ne_endpoint_update_local_address(endpoint);
                            }

                            net_ne_endpoint_do_read(driver, endpoint, base_endpoint);
                            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_established) != 0) {
                                net_endpoint_free(base_endpoint);
                                return;
                            }
                        }            
                        else if (net_endpoint_state(base_endpoint) == net_endpoint_state_established) {
                        }
                        else {
                            CPE_ERROR(
                                driver->m_em, "ne: %s: connected in state %s!",
                                net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint),
                                net_endpoint_state_str(net_endpoint_state(base_endpoint)));

                            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_established) != 0) {
                                net_endpoint_free(base_endpoint);
                                return;
                            }
                        }
                        break;
                    case NWTCPConnectionStateCancelled:
                        break;
                    case NWTCPConnectionStateInvalid:
                        break;
                    case NWTCPConnectionStateConnecting:
                    default:
                        break;
                    }
                }
            }
        });
}
@end
