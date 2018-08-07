#include "assert.h"
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils_sock/sock_utils.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_driver.h"
#include "net_dq_endpoint.h"

static void net_dq_endpoint_on_read(net_dq_endpoint_t endpoint);
static void net_dq_endpoint_on_write(net_dq_endpoint_t endpoint);
static void net_dq_endpoint_on_connect(net_dq_endpoint_t endpoint);
static void net_dq_endpoint_stop_w(net_dq_endpoint_t endpoint);
static void net_dq_endpoint_stop_r(net_dq_endpoint_t endpoint);

int net_dq_endpoint_init(net_endpoint_t base_endpoint) {
    net_dq_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    endpoint->m_fd = -1;
    endpoint->m_source_r = nil;
    endpoint->m_source_w = nil;

    return 0;
}

void net_dq_endpoint_fini(net_endpoint_t base_endpoint) {
    net_dq_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    net_dq_endpoint_stop_r(endpoint);
    net_dq_endpoint_stop_w(endpoint);

    if (endpoint->m_fd != -1) {
        cpe_sock_close(endpoint->m_fd);
        endpoint->m_fd = -1;
    }
}

int net_dq_endpoint_on_output(net_endpoint_t base_endpoint) {
    if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return 0;

    net_dq_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_dq_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    net_dq_endpoint_start_rw_watcher(driver, base_endpoint, endpoint);
    
    return 0;
}

int net_dq_endpoint_connect(net_endpoint_t base_endpoint) {
    net_dq_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_dq_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    if (endpoint->m_fd != -1) {
        CPE_ERROR(
            driver->m_em, "dq: %s: already connected!",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
        return -1;
    }

    net_address_t remote_addr = net_endpoint_remote_address(base_endpoint);
    if (remote_addr == NULL) {
        CPE_ERROR(
            driver->m_em, "dq: %s: connect with no remote address!",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
        return -1;
    }
        
    net_address_t local_address = net_endpoint_address(base_endpoint);
    if (local_address) {
        switch(net_address_type(local_address)) {
        case net_address_ipv4:
            endpoint->m_fd = cpe_sock_open(AF_INET, SOCK_STREAM, 0);
            break;
        case net_address_ipv6:
            endpoint->m_fd = cpe_sock_open(AF_INET6, SOCK_STREAM, 0);
            break;
        case net_address_domain:
            CPE_ERROR(
                driver->m_em, "dq: %s: connect not support domain address!",
                net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
            return -1;
        }

        if (endpoint->m_fd == -1) {
            CPE_ERROR(
                driver->m_em, "dq: %s: create socket fail, errno=%d (%s)",
                net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
            return -1;
        }

        struct sockaddr_storage addr;
        socklen_t addr_len = sizeof(addr);
        if (net_address_to_sockaddr(local_address, (struct sockaddr *)&addr, &addr_len) != 0) {
            CPE_ERROR(
                driver->m_em, "dq: %s: connect not support connect to domain address!",
                net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
            return -1;
        }

        if(cpe_bind(endpoint->m_fd, (struct sockaddr *)&addr, addr_len) != 0) {
            char local_addr_buf[128];
            cpe_str_dup(
                local_addr_buf, sizeof(local_addr_buf),
                net_address_dump(net_dq_driver_tmp_buffer(driver), local_address));

            CPE_ERROR(
                driver->m_em, "dq: %s: bind local address %s fail, errno=%d (%s)",
                local_addr_buf, net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));

            cpe_sock_close(endpoint->m_fd);
            endpoint->m_fd = -1;
            return -1;
        }
    }
    else {
        endpoint->m_fd = cpe_sock_open(AF_INET, SOCK_STREAM, 0);
        if (endpoint->m_fd == -1) {
            CPE_ERROR(
                driver->m_em, "dq: %s: create ipv4 socket fail, errno=%d (%s)",
                net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
            return -1;
        }
    }

    if (cpe_sock_set_none_block(endpoint->m_fd, 1) != 0) {
        CPE_ERROR(
            driver->m_em, "dq: %s: set non-block fail, errno=%d (%s)",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        cpe_sock_close(endpoint->m_fd);
        endpoint->m_fd = -1;
        return -1;
    }

    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    net_address_to_sockaddr(remote_addr, (struct sockaddr *)&addr, &addr_len);

    if (cpe_connect(endpoint->m_fd, (struct sockaddr *)&addr, addr_len) != 0) {
        if (cpe_sock_errno() == EINPROGRESS || cpe_sock_errno() == EWOULDBLOCK) {
            if (driver->m_debug) {
                if (local_address) {
                    char local_addr_buf[128];
                    cpe_str_dup(
                        local_addr_buf, sizeof(local_addr_buf),
                        net_address_dump(net_dq_driver_tmp_buffer(driver), local_address));

                    CPE_INFO(
                        driver->m_em, "dq: %s: connect start (local-address=%s)",
                        net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                        local_addr_buf);
                }
                else {
                    CPE_INFO(
                        driver->m_em, "dq: %s: connect start",
                        net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
                }
            }
            
            assert(endpoint->m_source_w == nil);
            endpoint->m_source_w = dispatch_source_create(DISPATCH_SOURCE_TYPE_WRITE, endpoint->m_fd, 0, dispatch_get_main_queue());
            dispatch_retain(endpoint->m_source_w);
            dispatch_source_set_event_handler(endpoint->m_source_w, ^{ net_dq_endpoint_on_connect(endpoint); });
            dispatch_resume(endpoint->m_source_w);
            
            return net_endpoint_set_state(base_endpoint, net_endpoint_state_connecting);
        }
        else {
            CPE_ERROR(
                driver->m_em, "dq: %s: connect error, errno=%d (%s)",
                net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
            cpe_sock_close(endpoint->m_fd);
            endpoint->m_fd = -1;
            return -1;
        }
    }
    else {
        if (driver->m_debug) {
            if (local_address) {
                char local_addr_buf[128];
                cpe_str_dup(
                    local_addr_buf, sizeof(local_addr_buf),
                    net_address_dump(net_dq_driver_tmp_buffer(driver), local_address));

                CPE_INFO(
                    driver->m_em, "dq: %s: connect success (local-address=%s)",
                    net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                    local_addr_buf);
            }
            else {
                CPE_INFO(
                    driver->m_em, "dq: %s: connect success",
                    net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
            }
        }

        if (net_endpoint_address(base_endpoint) == NULL) {
            net_dq_endpoint_update_local_address(endpoint);
        }

        net_dq_endpoint_start_rw_watcher(driver, base_endpoint, endpoint);
        
        return net_endpoint_set_state(base_endpoint, net_endpoint_state_established);
    }
}

void net_dq_endpoint_close(net_endpoint_t base_endpoint) {
    net_dq_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    net_dq_endpoint_stop_r(endpoint);
    net_dq_endpoint_stop_w(endpoint);
    
    if (endpoint->m_fd != -1) {
        cpe_sock_close(endpoint->m_fd);
        endpoint->m_fd = -1;
    }
}

void net_dq_endpoint_start_rw_watcher(
    net_dq_driver_t driver, net_endpoint_t base_endpoint, net_dq_endpoint_t endpoint)
{
    if (!net_endpoint_rbuf_is_full(base_endpoint)) {
        if (endpoint->m_source_r == nil) {
            endpoint->m_source_r = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, endpoint->m_fd, 0, dispatch_get_main_queue());
            dispatch_retain(endpoint->m_source_r);
            dispatch_source_set_event_handler(endpoint->m_source_r, ^{ net_dq_endpoint_on_read(endpoint); });
            dispatch_resume(endpoint->m_source_r);
        }
    }
    else {
        net_dq_endpoint_stop_r(endpoint);
    }

    if (!net_endpoint_wbuf_is_empty(base_endpoint)) {
        if (endpoint->m_source_w == nil) {
            endpoint->m_source_w = dispatch_source_create(DISPATCH_SOURCE_TYPE_WRITE, endpoint->m_fd, 0, dispatch_get_main_queue());
            dispatch_retain(endpoint->m_source_w);
            dispatch_source_set_event_handler(endpoint->m_source_w, ^{ net_dq_endpoint_on_write(endpoint); });
            dispatch_resume(endpoint->m_source_w);
        }
    }
    else {
        net_dq_endpoint_stop_w(endpoint);
    }
}

int net_dq_endpoint_update_local_address(net_dq_endpoint_t endpoint) {
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_dq_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(struct sockaddr_storage);
    memset(&addr, 0, addr_len);
    if (getsockname(endpoint->m_fd, (struct sockaddr *)&addr, &addr_len) != 0) {
        CPE_ERROR(
            driver->m_em, "dq: %s: sockaddr error, errno=%d (%s)",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        return -1;
    }

    net_address_t address =
        net_address_create_from_sockaddr(
            net_endpoint_schedule(base_endpoint), (struct sockaddr *)&addr, addr_len);
    if (address == NULL) {
        CPE_ERROR(
            driver->m_em, "dq: %s: create address fail",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
        return -1;
    }

    net_endpoint_set_address(base_endpoint, address, 1);
    
    return 0;
}

int net_dq_endpoint_update_remote_address(net_dq_endpoint_t endpoint) {
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_dq_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(struct sockaddr_storage);
    memset(&addr, 0, addr_len);
    if (getpeername(endpoint->m_fd, (struct sockaddr *)&addr, &addr_len) != 0) {
        CPE_ERROR(
            driver->m_em, "dq: %s: getpeername error, errno=%d (%s)",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        return -1;
    }

    net_address_t address = net_address_create_from_sockaddr(
        net_endpoint_schedule(base_endpoint), (struct sockaddr *)&addr, addr_len);
    if (address == NULL) {
        CPE_ERROR(
            driver->m_em, "dq: %s: create address fail",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
        return -1;
    }

    net_endpoint_set_remote_address(base_endpoint, address, 1);
    
    return 0;
}

static void net_dq_endpoint_on_read(net_dq_endpoint_t endpoint) {
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_dq_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    net_dq_endpoint_stop_r(endpoint);

    for(;net_endpoint_state(base_endpoint) == net_endpoint_state_established;) {
        uint32_t capacity = 0;
        void * rbuf = net_endpoint_rbuf_alloc(base_endpoint, &capacity);
        if (rbuf == NULL) {
            assert(net_endpoint_rbuf_is_full(base_endpoint));
            break;
        }
            
        ssize_t bytes = cpe_recv(endpoint->m_fd, rbuf, capacity, 0);
        if (bytes > 0) {
            if (driver->m_debug) {
                CPE_INFO(
                    driver->m_em, "dq: %s: recv %d bytes data!",
                    net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                    (int)bytes);
            }

            if (net_endpoint_rbuf_supply(base_endpoint, (uint32_t)bytes) != 0) {
                if (net_endpoint_set_state(base_endpoint, net_endpoint_state_logic_error) != 0) {
                    if (driver->m_debug) {
                        CPE_INFO(
                            driver->m_em, "dq: %s: free for process fail!",
                            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
                    }
                    net_endpoint_free(base_endpoint);
                }
                return;
            }

            if (driver->m_data_monitor_fun) {
                driver->m_data_monitor_fun(driver->m_data_monitor_ctx, base_endpoint, net_data_in, (uint32_t)bytes);
            }

            break;
        }
        else if (bytes == 0) {
            if (driver->m_debug) {
                CPE_INFO(
                    driver->m_em, "dq: %s: remote disconnected(recv 0)!",
                    net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
            }
                
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_disable) != 0) {
                net_endpoint_free(base_endpoint);
            }
            return;
        }
        else {
            assert(bytes == -1);

            switch(errno) {
            case EWOULDBLOCK:
            case EINPROGRESS:
                break;
            case EINTR:
                continue;
            default:
                CPE_ERROR(
                    driver->m_em, "dq: %s: recv error, errno=%d (%s)!",
                    net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                    cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));

                if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
                    net_endpoint_free(base_endpoint);
                }
                return;
            }
        }
    }

    if (net_endpoint_state(base_endpoint) == net_endpoint_state_established) {
        net_dq_endpoint_start_rw_watcher(driver, base_endpoint, endpoint);
    }
}

static void net_dq_endpoint_on_write(net_dq_endpoint_t endpoint) {
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_dq_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    net_dq_endpoint_stop_w(endpoint);

    while(net_endpoint_state(base_endpoint) == net_endpoint_state_established && !net_endpoint_wbuf_is_empty(base_endpoint)) {
        uint32_t data_size;
        void * data = net_endpoint_wbuf(base_endpoint, &data_size);

        assert(data_size > 0);
        assert(data);

        ssize_t bytes = cpe_send(endpoint->m_fd, data, data_size, 0);
        if (bytes > 0) {
            if (driver->m_debug) {
                CPE_INFO(
                    driver->m_em, "dq: %s: send %d bytes data!",
                    net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                    (int)bytes);
            }

            net_endpoint_wbuf_consume(base_endpoint, (uint32_t)bytes);

            if (driver->m_data_monitor_fun) {
                driver->m_data_monitor_fun(driver->m_data_monitor_ctx, base_endpoint, net_data_out, (uint32_t)bytes);
            }
        }
        else if (bytes == 0) {
            if (driver->m_debug) {
                CPE_INFO(
                    driver->m_em, "dq: %s: free for send return 0!",
                    net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
            }

            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
                net_endpoint_free(base_endpoint);
            }
            return;
        }
        else {
            int err = cpe_sock_errno();
            assert(bytes == -1);

            if (err == EWOULDBLOCK || err == EINPROGRESS) break;
            if (err == EINTR) continue;

            CPE_ERROR(
                driver->m_em, "dq: %s: free for send error, errno=%d (%s)!",
                net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));

            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
                net_endpoint_free(base_endpoint);
            }
            return;
        }
    }

    if (net_endpoint_state(base_endpoint) == net_endpoint_state_established) {
        net_dq_endpoint_start_rw_watcher(driver, base_endpoint, endpoint);
    }
}

static void net_dq_endpoint_on_connect(net_dq_endpoint_t endpoint) {
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_dq_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    net_dq_endpoint_stop_w(endpoint);

    int err;
    socklen_t err_len;
    if (cpe_getsockopt(endpoint->m_fd, SOL_SOCKET, SO_ERROR, (void*)&err, &err_len) == -1) {
        CPE_ERROR(
            driver->m_em, "dq: %s: connect_cb get socket error fail, errno=%d (%s)!",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));

        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
            net_endpoint_free(base_endpoint);
        }
        return;
    }

    if (err != 0) {
        CPE_ERROR(
            driver->m_em, "dq: %s: connect error, errno=%d (%s)",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
            net_endpoint_free(base_endpoint);
        }
        return;
    }

    if (driver->m_debug) {
        CPE_INFO(
            driver->m_em, "dq: %s: connect success",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
    }

    if (net_endpoint_address(base_endpoint) == NULL) {
        net_dq_endpoint_update_local_address(endpoint);
    }

    net_dq_endpoint_start_rw_watcher(driver, base_endpoint, endpoint);
    if (net_endpoint_set_state(base_endpoint, net_endpoint_state_established) != 0) {
        net_endpoint_free(base_endpoint);
        return;
    }
}

static void net_dq_endpoint_stop_w(net_dq_endpoint_t endpoint) {
    if (endpoint->m_source_w) {
        dispatch_source_set_event_handler(endpoint->m_source_w, nil);
        dispatch_source_cancel(endpoint->m_source_w);
        dispatch_release(endpoint->m_source_w);
        endpoint->m_source_w = nil;
    }
}

static void net_dq_endpoint_stop_r(net_dq_endpoint_t endpoint) {
    if (endpoint->m_source_r) {
        dispatch_source_set_event_handler(endpoint->m_source_r, nil);
        dispatch_source_cancel(endpoint->m_source_r);
        dispatch_release(endpoint->m_source_r);
        endpoint->m_source_r = nil;
    }
}
