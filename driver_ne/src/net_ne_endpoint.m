#include "assert.h"
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_driver.h"
#include "net_ne_endpoint.h"
#include "net_ne_utils.h"

static const char * net_ne_endpoint_state_str(NWTCPConnectionState state);

int net_ne_endpoint_init(net_endpoint_t base_endpoint) {
    net_ne_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_ne_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    endpoint->m_connection = nil;
    endpoint->m_observer = [[[NetNeEndpointObserver alloc] init] retain];

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
        [endpoint->m_connection release];
        endpoint->m_connection = nil;
    }

    [endpoint->m_observer release];
    endpoint->m_observer = nil;
}

int net_ne_endpoint_on_output(net_endpoint_t base_endpoint) {
    if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return 0;

    net_ne_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_ne_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    CPE_ERROR(
        driver->m_em, "ne: %s: on output",
        net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint));

    return 0;
}

int net_ne_endpoint_connect(net_endpoint_t base_endpoint) {
    net_ne_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_ne_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);

    CPE_ERROR(
        driver->m_em, "ne: %s: connect begin",
        net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint));

    @autoreleasepool {
        if (endpoint->m_connection != nil) {
            CPE_ERROR(
                driver->m_em, "ne: %s: already connected!",
                net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint));
            return -1;
        }

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

// static void net_ne_endpoint_rw_cb(EV_P_ ev_io *w, int revents) {
//     net_ne_endpoint_t endpoint = w->data;
//     net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
//     net_ne_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
//     net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
//     error_monitor_t em = net_schedule_em(schedule);

//     if (revents & EV_READ) {
//         for(;net_endpoint_state(base_endpoint) == net_endpoint_state_established;) {
//             uint32_t capacity = 0;
//             void * rbuf = net_endpoint_rbuf_alloc(base_endpoint, &capacity);
//             if (rbuf == NULL) {
//                 assert(net_endpoint_rbuf_is_full(base_endpoint));
//                 break;
//             }
            
//             int bytes = cpe_recv(endpoint->m_fd, rbuf, capacity, 0);
//             if (bytes > 0) {
//                 if (driver->m_debug) {
//                     CPE_INFO(
//                         em, "ne: %s: recv %d bytes data!",
//                         net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint),
//                         bytes);
//                 }

//                 if (net_endpoint_rbuf_supply(base_endpoint, (uint32_t)bytes) != 0) {
//                     if (net_endpoint_set_state(base_endpoint, net_endpoint_state_logic_error) != 0) {
//                         if (driver->m_debug || net_schedule_debug(schedule) >= 2) {
//                             CPE_INFO(
//                                 em, "ne: %s: free for process fail!",
//                                 net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint));
//                         }
//                         net_endpoint_free(base_endpoint);
//                     }
//                     return;
//                 }

//                 if (driver->m_data_monitor_fun) {
//                     driver->m_data_monitor_fun(driver->m_data_monitor_ctx, base_endpoint, net_data_in, (uint32_t)bytes);
//                 }

//                 break;
//             }
//             else if (bytes == 0) {
//                 if (driver->m_debug || net_schedule_debug(schedule) >= 2) {
//                     CPE_INFO(
//                         em, "ne: %s: remote disconnected(recv 0)!",
//                         net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint));
//                 }
                
//                 if (net_endpoint_set_state(base_endpoint, net_endpoint_state_disable) != 0) {
//                     net_endpoint_free(base_endpoint);
//                 }
//                 return;
//             }
//             else {
//                 assert(bytes == -1);

//                 switch(errno) {
//                 case EWOULDBLOCK:
//                 case EINPROGRESS:
//                     break;
//                 case EINTR:
//                     continue;
//                 default:
//                     CPE_ERROR(
//                         em, "ne: %s: recv error, errno=%d (%s)!",
//                         net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint),
//                         cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));

//                     if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
//                         net_endpoint_free(base_endpoint);
//                     }
//                     return;
//                 }
//             }
//         }
//     }

//     if (revents & EV_WRITE) {
//         while(net_endpoint_state(base_endpoint) == net_endpoint_state_established && !net_endpoint_wbuf_is_empty(base_endpoint)) {
//             uint32_t data_size;
//             void * data = net_endpoint_wbuf(base_endpoint, &data_size);

//             assert(data_size > 0);
//             assert(data);

//             int bytes = cpe_send(endpoint->m_fd, data, data_size, 0);
//             if (bytes > 0) {
//                 if (driver->m_debug) {
//                     CPE_INFO(
//                         em, "ne: %s: send %d bytes data!",
//                         net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint),
//                         bytes);
//                 }

//                 net_endpoint_wbuf_consume(base_endpoint, (uint32_t)bytes);

//                 if (driver->m_data_monitor_fun) {
//                     driver->m_data_monitor_fun(driver->m_data_monitor_ctx, base_endpoint, net_data_out, (uint32_t)bytes);
//                 }
//             }
//             else if (bytes == 0) {
//                 if (driver->m_debug || net_schedule_debug(schedule) >= 2) {
//                     CPE_INFO(
//                         em, "ne: %s: free for send return 0!",
//                         net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint));
//                 }

//                 net_endpoint_free(base_endpoint);
//                 return;
//             }
//             else {
//                 int err = cpe_sock_errno();
//                 assert(bytes == -1);

//                 if (err == EWOULDBLOCK || err == EINPROGRESS) break;
//                 if (err == EINTR) continue;

//                 CPE_ERROR(
//                     em, "ne: %s: free for send error, errno=%d (%s)!",
//                     net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint),
//                     cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));

//                 net_endpoint_free(base_endpoint);
//                 return;
//             }
//         }
//     }
// }

// static void net_ne_endpoint_connect_cb(EV_P_ ev_io *w, int revents) {
//     net_ne_endpoint_t endpoint = w->data;
//     net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
//     net_ne_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
//     net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
//     error_monitor_t em = net_schedule_em(schedule);

//     ev_io_stop(driver->m_ne_loop, &endpoint->m_watcher);

//     int err;
//     socklen_t err_len;
//     if (cpe_getsockopt(endpoint->m_fd, SOL_SOCKET, SO_ERROR, (void*)&err, &err_len) == -1) {
//         CPE_ERROR(
//             em, "ne: %s: connect_cb get socket error fail, errno=%d (%s)!",
//             net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint),
//             cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));

//         if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
//             net_endpoint_free(base_endpoint);
//         }
//         return;
//     }

//     if (err != 0) {
//         CPE_ERROR(
//             em, "ne: %s: connect error, errno=%d (%s)",
//             net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint),
//             cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
//         if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
//             net_endpoint_free(base_endpoint);
//         }
//         return;
//     }

//     if (driver->m_debug || net_schedule_debug(schedule) >= 2) {
//         CPE_INFO(
//             em, "ne: %s: connect success",
//             net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint),
//             cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
//     }

//     if (net_endpoint_address(base_endpoint) == NULL) {
//         net_ne_endpoint_update_local_address(endpoint);
//     }

//     net_ne_endpoint_start_rw_watcher(driver, base_endpoint, endpoint);
//     if (net_endpoint_set_state(base_endpoint, net_endpoint_state_established) != 0) {
//         net_endpoint_free(base_endpoint);
//         return;
//     }
// }

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
    default:
        return "Unknown";
    }
}

@implementation NetNeEndpointObserver
-(void)observeValueForKeyPath:(NSString *)keyPath 
                     ofObject:(id)object 
                       change:(NSDictionary *)change 
                      context:(void *)context
{
    net_ne_endpoint_t endpoint = context;
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_ne_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    if ([keyPath isEqual:@"state"]) {
        CPE_INFO(
            driver->m_em, "ne: %s: state %s ==> %s!",
            net_endpoint_dump(net_ne_driver_tmp_buffer(driver), base_endpoint),
            net_ne_endpoint_state_str((NWTCPConnectionState)[change objectForKey:@"old"]),
            net_ne_endpoint_state_str((NWTCPConnectionState)[change objectForKey:@"new"]));
    }
}
@end
