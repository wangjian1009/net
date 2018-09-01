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

static void net_dq_endpoint_on_connect(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint);

static void net_dq_endpoint_start_w(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint);
static void net_dq_endpoint_stop_w(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint);
static void net_dq_endpoint_start_r(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint);
static void net_dq_endpoint_stop_r(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint);

int net_dq_endpoint_init(net_endpoint_t base_endpoint) {
    net_dq_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    endpoint->m_fd = -1;
    endpoint->m_source_r = nil;
    endpoint->m_source_w = nil;

    return 0;
}

void net_dq_endpoint_fini(net_endpoint_t base_endpoint) {
    net_dq_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_dq_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    if (endpoint->m_source_r) {
        net_dq_endpoint_stop_r(driver, endpoint, base_endpoint);
    }

    if (endpoint->m_source_w) {
        net_dq_endpoint_stop_w(driver, endpoint, base_endpoint);
    }

    if (endpoint->m_fd != -1) {
        cpe_sock_close(endpoint->m_fd);
        endpoint->m_fd = -1;
    }
}

int net_dq_endpoint_on_output(net_endpoint_t base_endpoint) {
    net_dq_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_dq_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    return net_dq_endpoint_on_write(driver, endpoint, base_endpoint);
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
    remote_addr = net_address_resolved(remote_addr);

    struct sockaddr_storage remote_addr_sock;
    socklen_t remote_addr_sock_len = sizeof(remote_addr_sock);
    net_address_to_sockaddr(remote_addr, (struct sockaddr *)&remote_addr_sock, &remote_addr_sock_len);

    int domain;
    switch(net_address_type(remote_addr)) {
    case net_address_ipv4:
        domain = AF_INET;
        break;
    case net_address_ipv6:
        domain = AF_INET6;
        break;
    case net_address_domain:
        CPE_ERROR(
            driver->m_em, "dq: %s: connect not support domain address!",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
        return -1;
    }

CREATE_SOCKET:
    endpoint->m_fd = cpe_sock_open(domain, SOCK_STREAM, 0);
    if (endpoint->m_fd == -1) {
        CPE_ERROR(
            driver->m_em, "dq: %s: create socket fail, errno=%d (%s)",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        return -1;
    }

    struct sockaddr_storage local_addr_sock;
    socklen_t local_addr_sock_len;
    bzero(&local_addr_sock, sizeof(local_addr_sock));
    
    net_address_t local_address = net_endpoint_address(base_endpoint);
    if (local_address) {
        local_addr_sock_len = sizeof(local_addr_sock);

        if (net_address_to_sockaddr(local_address, (struct sockaddr *)&local_addr_sock, &local_addr_sock_len) != 0) {
            CPE_ERROR(
                driver->m_em, "dq: %s: connect not support connect to domain address!",
                net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
            return -1;
        }
    }
    else {
        if (domain == AF_INET) {
            struct sockaddr_in * s = (struct sockaddr_in *)&local_addr_sock;
            s->sin_len         = sizeof(*s);
            s->sin_family      = AF_INET;
            s->sin_port        = 0;
            s->sin_addr.s_addr = htonl(INADDR_ANY);
            local_addr_sock_len = sizeof(*s);
        }
        else {
            assert(domain == AF_INET6);

            struct sockaddr_in6 * s = (struct sockaddr_in6 *)&local_addr_sock;
            s->sin6_len       = sizeof(*s);
            s->sin6_family    = AF_INET6;
            s->sin6_port      = 0;
            s->sin6_addr      = in6addr_any;
            local_addr_sock_len = sizeof(*s);
        }
    }

    if (cpe_sock_set_reuseaddr(endpoint->m_fd, 1) != 0) {
        CPE_ERROR(
            driver->m_em, "dq: %s: set sock reuse address fail, errno=%d (%s)",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        cpe_sock_close(endpoint->m_fd);
        endpoint->m_fd = -1;
        return -1;
    }

    if(cpe_bind(endpoint->m_fd, (struct sockaddr *)&local_addr_sock, local_addr_sock_len) != 0) {
        char local_addr_buf[128];
        cpe_str_dup(
            local_addr_buf, sizeof(local_addr_buf),
            net_address_dump(net_dq_driver_tmp_buffer(driver), local_address));

        CPE_ERROR(
            driver->m_em, "dq: %s: bind local address %s fail, errno=%d (%s)",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint), local_addr_buf,
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));

        cpe_sock_close(endpoint->m_fd);
        endpoint->m_fd = -1;
        return -1;
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

    if (cpe_sock_set_no_sigpipe(endpoint->m_fd, 1) != 0) {
        CPE_ERROR(
            driver->m_em, "dq: %s: set no-sig-pipe fail, errno=%d (%s)",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        cpe_sock_close(endpoint->m_fd);
        endpoint->m_fd = -1;
        return -1;
    }

    if (cpe_connect(endpoint->m_fd, (struct sockaddr *)&remote_addr_sock, remote_addr_sock_len) != 0) {
        // char addr_buf[INET6_ADDRSTRLEN + 32];

        // if (remote_addr_sock.ss_family == AF_INET) {
        //     inet_ntop(AF_INET, &((struct sockaddr_in *)&remote_addr_sock)->sin_addr, addr_buf, sizeof(addr_buf));
        // }
        // else {
        //     assert(remote_addr_sock.ss_family == AF_INET6);
        //     inet_ntop(AF_INET6, &((struct sockaddr_in6 *)&remote_addr_sock)->sin6_addr, addr_buf, sizeof(addr_buf));
        // }
        
        // CPE_ERROR(
        //     driver->m_em, "dq: %s: xxxxxxx: domain=%s, ip=%s, errno=%d (%s), ENETUNREACH=%d (%s)",
        //     net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
        //     (domain == AF_INET6 ? "ipv6" : domain == AF_INET ? "ipv4" : "unknown"),
        //     addr_buf,
        //     cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()),
        //     ENETUNREACH, cpe_sock_errstr(ENETUNREACH));
        
        if (cpe_sock_errno() == ENETUNREACH) {
            if (domain == AF_INET && net_address_type(remote_addr) == net_address_ipv4) {
                /*ipv4网络不可达，尝试ipv6 */

                net_address_t remote_addr_ipv6 = net_address_create_ipv6_from_ipv4(net_endpoint_schedule(base_endpoint), remote_addr);
                if (remote_addr_ipv6 == NULL) {
                    CPE_ERROR(
                        driver->m_em, "dq: %s: ipv4 network unreachable, switch to ipv6, create address fail",
                        net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
                    cpe_sock_close(endpoint->m_fd);
                    endpoint->m_fd = -1;
                    return -1;
                }

                if (net_endpoint_driver_debug(base_endpoint) >= 0) {
                    char addr_buf[INET6_ADDRSTRLEN + 32];
                    cpe_str_dup(
                        addr_buf, sizeof(addr_buf),
                        net_address_dump(net_dq_driver_tmp_buffer(driver), remote_addr_ipv6));
                    
                    CPE_INFO(
                        driver->m_em, "dq: %s: ipv4 network unreachable, try ipv6 %s!",
                        net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                        addr_buf);
                }

                domain = AF_INET6;
                
                remote_addr_sock_len = sizeof(remote_addr_sock);
                net_address_to_sockaddr(remote_addr_ipv6, (struct sockaddr *)&remote_addr_sock, &remote_addr_sock_len);
                net_address_free(remote_addr_ipv6);
                
                cpe_sock_close(endpoint->m_fd);
                endpoint->m_fd = -1;

                goto CREATE_SOCKET;
            }
            else if (domain == AF_INET6 && net_address_type(remote_addr) == net_address_ipv6 && net_address_ipv6_is_wrap_ipv4(remote_addr)) {
                /*ipv6网络不可达，且是一个ipv4地址的封装，尝试ipv4网络 */
                net_address_t remote_addr_ipv4 = net_address_create_ipv4_from_ipv6_wrap(net_endpoint_schedule(base_endpoint), remote_addr);
                if (remote_addr_ipv4 == NULL) {
                    CPE_ERROR(
                        driver->m_em, "dq: %s: ipv6 network unreachable, try ipv4, create address fail",
                        net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
                    cpe_sock_close(endpoint->m_fd);
                    endpoint->m_fd = -1;
                    return -1;
                }

                if (net_endpoint_driver_debug(base_endpoint) >= 0) {
                    char addr_buf[INET6_ADDRSTRLEN + 32];
                    cpe_str_dup(
                        addr_buf, sizeof(addr_buf),
                        net_address_dump(net_dq_driver_tmp_buffer(driver), remote_addr_ipv4));
                    
                    CPE_INFO(
                        driver->m_em, "dq: %s: ipv6 network unreachable, try ipv4 %s!",
                        net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                        addr_buf);
                }
                
                domain = AF_INET;
                
                remote_addr_sock_len = sizeof(remote_addr_sock);
                net_address_to_sockaddr(remote_addr_ipv4, (struct sockaddr *)&remote_addr_sock, &remote_addr_sock_len);
                net_address_free(remote_addr_ipv4);
                
                cpe_sock_close(endpoint->m_fd);
                endpoint->m_fd = -1;
                
                goto CREATE_SOCKET;
            }
        }

        if (cpe_sock_errno() == EINPROGRESS || cpe_sock_errno() == EWOULDBLOCK) {
            if (net_endpoint_driver_debug(base_endpoint)) {
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
            dispatch_source_set_event_handler(endpoint->m_source_w, ^{
                    net_dq_endpoint_stop_w(driver, endpoint, base_endpoint);
                    net_dq_endpoint_on_connect(driver, endpoint, base_endpoint);
                });
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
        if (net_endpoint_driver_debug(base_endpoint)) {
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

        return net_dq_endpoint_set_established(driver, endpoint, base_endpoint);
    }
}

void net_dq_endpoint_close(net_endpoint_t base_endpoint) {
    net_dq_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_dq_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    if (endpoint->m_source_r) {
        net_dq_endpoint_stop_r(driver, endpoint, base_endpoint);
    }

    if (endpoint->m_source_w) {
        net_dq_endpoint_stop_w(driver, endpoint, base_endpoint);
    }

    if (endpoint->m_fd != -1) {
        cpe_sock_close(endpoint->m_fd);
        endpoint->m_fd = -1;
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

int net_dq_endpoint_set_established(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint) {
    assert(endpoint->m_source_r == NULL);
    assert(endpoint->m_source_w == NULL);

    if (net_endpoint_set_state(base_endpoint, net_endpoint_state_established) != 0) return -1;

    if (net_dq_endpoint_on_read(driver, endpoint, base_endpoint) != 0) return -1;
    
    if (net_dq_endpoint_on_write(driver, endpoint, base_endpoint) != 0) return -1;

    return 0;
}

int net_dq_endpoint_on_read(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint) {
    while(
        endpoint->m_source_r == NULL /*can read*/
        && net_endpoint_state(base_endpoint) == net_endpoint_state_established
        )
    {
        uint32_t capacity = 0;
        void * rbuf = net_endpoint_buf_alloc(base_endpoint, &capacity);
        if (rbuf == NULL) {
            CPE_ERROR(
                driver->m_em, "dq: %s: on read: endpoint rbuf full!",
                net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) return -1;
            return 0;
        }

        ssize_t bytes = cpe_recv(endpoint->m_fd, rbuf, capacity, 0);
        if (bytes > 0) {
            if (net_endpoint_driver_debug(base_endpoint)) {
                CPE_INFO(
                    driver->m_em, "dq: %s: recv %d bytes data!",
                    net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                    (int)bytes);
            }

            if (net_endpoint_buf_supply(base_endpoint, net_ep_buf_read, (uint32_t)bytes) != 0) {
                if (net_endpoint_set_state(base_endpoint, net_endpoint_state_logic_error) != 0) {
                    if (net_endpoint_driver_debug(base_endpoint)) {
                        CPE_INFO(
                            driver->m_em, "dq: %s: free for process fail!",
                            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
                    }
                    return -1;
                }
            }

            if (driver->m_data_monitor_fun) {
                driver->m_data_monitor_fun(driver->m_data_monitor_ctx, base_endpoint, net_data_in, (uint32_t)bytes);
            }

            continue;
        }
        else if (bytes == 0) {
            net_endpoint_buf_release(base_endpoint);
            if (net_endpoint_driver_debug(base_endpoint)) {
                CPE_INFO(
                    driver->m_em, "dq: %s: remote disconnected(recv 0)!",
                    net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
            }
                
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_disable) != 0) return -1;
            return 0;
        }
        else {
            net_endpoint_buf_release(base_endpoint);
            assert(bytes == -1);

            switch(errno) {
            case EWOULDBLOCK:
            case EINPROGRESS:
                net_dq_endpoint_start_r(driver, endpoint, base_endpoint);
                return 0;
            case EINTR:
                continue;
            default:
                CPE_ERROR(
                    driver->m_em, "dq: %s: recv error, errno=%d (%s)!",
                    net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                    cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));

                if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) return -1;
                return 0;
            }
        }
        assert(0);
    }

    return 0;
}

int net_dq_endpoint_on_write(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint) {
    while(endpoint->m_source_w == NULL /*can write*/
          && net_endpoint_state(base_endpoint) == net_endpoint_state_established
          && !net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write)
        )
    {
        uint32_t data_size = net_endpoint_buf_size(base_endpoint, net_ep_buf_write);
        void * data = net_endpoint_buf_peak(base_endpoint, net_ep_buf_write, &data_size);
        assert(data_size > 0);
        assert(data);

        ssize_t bytes = cpe_send(endpoint->m_fd, data, data_size, 0);
        if (bytes > 0) {
            if (net_endpoint_driver_debug(base_endpoint)) {
                CPE_INFO(
                    driver->m_em, "dq: %s: send %d/%d bytes data!",
                    net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                    (int)bytes, data_size);
            }

            net_endpoint_buf_consume(base_endpoint, net_ep_buf_write, (uint32_t)bytes);

            if (driver->m_data_monitor_fun) {
                driver->m_data_monitor_fun(driver->m_data_monitor_ctx, base_endpoint, net_data_out, (uint32_t)bytes);
            }
        }
        else if (bytes == 0) {
            if (net_endpoint_driver_debug(base_endpoint)) {
                CPE_INFO(
                    driver->m_em, "dq: %s: free for send return 0!",
                    net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
            }

            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) return -1;
            return 0;
        }
        else {
            int err = cpe_sock_errno();
            assert(bytes == -1);

            if (err == EWOULDBLOCK || err == EINPROGRESS) {
                net_dq_endpoint_start_w(driver, endpoint, base_endpoint);
                return 0;
            }
        
            if (err == EINTR) continue;

            if (err == EPIPE) {
                if (net_endpoint_driver_debug(base_endpoint)) {
                    CPE_INFO(
                        driver->m_em, "dq: %s: free for send recv EPIPE!",
                        net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
                }

                if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) return -1;
                return 0;
            }
            
            CPE_ERROR(
                driver->m_em, "dq: %s: send error, errno=%d (%s)!",
                net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));

            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) return -1;

            return 0;
        }
    }
    
    return 0;
}

static void net_dq_endpoint_on_connect(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint) {
    int err;
    socklen_t err_len = sizeof(err);
    if (cpe_getsockopt(endpoint->m_fd, SOL_SOCKET, SO_ERROR, (void*)&err, &err_len) == -1) {
        CPE_ERROR(
            driver->m_em, "dq: %s: connect: connect get socket error fail, errno=%d (%s)!",
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

    if (net_endpoint_driver_debug(base_endpoint)) {
        CPE_INFO(
            driver->m_em, "dq: %s: connect success",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
    }

    if (net_endpoint_address(base_endpoint) == NULL) {
        net_dq_endpoint_update_local_address(endpoint);
    }

    if (net_dq_endpoint_set_established(driver, endpoint, base_endpoint) != 0) {
        net_endpoint_free(base_endpoint);
        return;
    }
}

static void net_dq_endpoint_start_w(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint) {
    assert(endpoint->m_source_w == NULL);
    
    if (net_endpoint_driver_debug(base_endpoint) >= 2) {
        CPE_INFO(
            driver->m_em, "dq: %s: start write watcher",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
    }
    
    endpoint->m_source_w = dispatch_source_create(DISPATCH_SOURCE_TYPE_WRITE, endpoint->m_fd, 0, dispatch_get_main_queue());
    dispatch_source_set_event_handler(endpoint->m_source_w, ^{
            net_dq_endpoint_stop_w(driver, endpoint, base_endpoint);
            if (net_dq_endpoint_on_write(driver, endpoint, base_endpoint) != 0) {
                net_endpoint_free(base_endpoint);
            }
        });
    dispatch_resume(endpoint->m_source_w);
}
    
static void net_dq_endpoint_stop_w(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint) {
    assert(endpoint->m_source_w != NULL);
    
    if (net_endpoint_driver_debug(base_endpoint) >= 2) {
        CPE_INFO(
            driver->m_em, "dq: %s: stop write watcher",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
    }

    dispatch_source_set_event_handler(endpoint->m_source_w, nil);
    dispatch_source_cancel(endpoint->m_source_w);
    dispatch_release(endpoint->m_source_w);
    endpoint->m_source_w = nil;
}

static void net_dq_endpoint_start_r(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint) {
    assert(endpoint->m_source_r == NULL);

    if (net_endpoint_driver_debug(base_endpoint) >= 2) {
        CPE_INFO(
            driver->m_em, "dq: %s: start read watcher",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
    }
    
    endpoint->m_source_r = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, endpoint->m_fd, 0, dispatch_get_main_queue());
    dispatch_source_set_event_handler(endpoint->m_source_r, ^{
            net_dq_endpoint_stop_r(driver, endpoint, base_endpoint);
            if (net_dq_endpoint_on_read(driver, endpoint, base_endpoint) != 0) {
                net_endpoint_free(base_endpoint);
            }
        });
    dispatch_resume(endpoint->m_source_r);
    
}

static void net_dq_endpoint_stop_r(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint) {
    assert(endpoint->m_source_r != NULL);

    if (net_endpoint_driver_debug(base_endpoint) >= 2) {
        CPE_INFO(
            driver->m_em, "dq: %s: stop read watcher",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
    }
    
    dispatch_source_set_event_handler(endpoint->m_source_r, nil);
    dispatch_source_cancel(endpoint->m_source_r);
    dispatch_release(endpoint->m_source_r);
    endpoint->m_source_r = nil;
}
