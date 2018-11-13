#include "assert.h"
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils_sock/sock_utils.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_driver.h"
#include "net_ev_endpoint.h"

static void net_ev_endpoint_rw_cb(EV_P_ ev_io *w, int revents);
static void net_ev_endpoint_connect_cb(EV_P_ ev_io *w, int revents);

static int net_ev_endpoint_start_connect(
    net_ev_driver_t driver, net_ev_endpoint_t endpoint, net_endpoint_t base_endpoint, net_address_t remote_address);

static void net_ev_endpoint_connect_log_connect_start(
    net_ev_driver_t driver, net_ev_endpoint_t endpoint, net_endpoint_t base_endpoint, uint8_t is_first);
static void net_ev_endpoint_connect_log_connect_success(
    net_ev_driver_t driver, net_ev_endpoint_t endpoint, net_endpoint_t base_endpoint);
static void net_ev_endpoint_connect_log_connect_error(
    net_ev_driver_t driver, net_ev_endpoint_t endpoint, net_endpoint_t base_endpoint, int err, uint8_t is_first);

static void net_ev_endpoint_close_sock(net_ev_driver_t driver, net_ev_endpoint_t endpoint);

int net_ev_endpoint_init(net_endpoint_t base_endpoint) {
    net_ev_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    
    endpoint->m_fd = -1;
    bzero(&endpoint->m_watcher, sizeof(endpoint->m_watcher));

    endpoint->m_watcher.data = endpoint;

    return 0;
}

void net_ev_endpoint_fini(net_endpoint_t base_endpoint) {
    net_ev_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_ev_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    if (endpoint->m_fd != -1) {
        ev_io_stop(driver->m_ev_loop, &endpoint->m_watcher);
        net_ev_endpoint_close_sock(driver, endpoint);
    }
}

int net_ev_endpoint_on_output(net_endpoint_t base_endpoint) {
    if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return 0;

    net_ev_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_ev_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    net_ev_endpoint_start_rw_watcher(driver, base_endpoint, endpoint);
    return 0;
}

int net_ev_endpoint_connect(net_endpoint_t base_endpoint) {
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_ev_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_ev_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    if (endpoint->m_fd != -1) {
        CPE_ERROR(
            driver->m_em, "ev: %s: fd=%d: already connected!",
            net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
        return -1;
    }

    net_address_t remote_addr = net_endpoint_remote_address(base_endpoint);
    if (remote_addr == NULL) {
        CPE_ERROR(
            driver->m_em, "ev: %s: fd=%d: connect with no remote address!",
            net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
        return -1;
    }
    remote_addr = net_address_resolved(remote_addr);

    if (net_address_type(remote_addr) != net_address_ipv4 && net_address_type(remote_addr) != net_address_ipv6) {
        CPE_ERROR(
            driver->m_em, "ev: %s: fd=%d: connect not support domain address!",
            net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
        return -1;
    }
    
    int connect_rv;

    net_local_ip_stack_t ip_stack = net_schedule_local_ip_stack(schedule);
    switch(ip_stack) {
    case net_local_ip_stack_none:
        CPE_ERROR(
            driver->m_em, "ev: %s: fd=%d: can`t connect in %s!",
            net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            net_local_ip_stack_str(ip_stack));
        return -1;
    case net_local_ip_stack_ipv4:
        if (net_address_type(remote_addr) == net_address_ipv6) {
            if (net_address_ipv6_is_ipv4_map(remote_addr)) {
                net_address_t remote_addr_ipv4 = net_address_create_ipv4_from_ipv6_map(net_endpoint_schedule(base_endpoint), remote_addr);
                if (remote_addr_ipv4 == NULL) {
                    CPE_ERROR(
                        driver->m_em, "ev: %s: fd=%d: convert ipv6 address to ipv4(map) fail",
                        net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
                    return -1;
                }

                connect_rv = net_ev_endpoint_start_connect(driver, endpoint, base_endpoint, remote_addr_ipv4);

                net_address_free(remote_addr_ipv4);
            }
            else {
                CPE_ERROR(
                    driver->m_em, "ev: %s: fd=%d: can`t connect to ipv6 network in ipv4 network env!",
                    net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
                return -1;
            }
        }
        else {
            connect_rv = net_ev_endpoint_start_connect(driver, endpoint, base_endpoint, remote_addr);
        }
        break;
    case net_local_ip_stack_ipv6:
        if (net_address_type(remote_addr) == net_address_ipv4) {
            net_address_t remote_addr_ipv6 = net_address_create_ipv6_from_ipv4_nat(net_endpoint_schedule(base_endpoint), remote_addr);
            if (remote_addr_ipv6 == NULL) {
                CPE_ERROR(
                    driver->m_em, "ev: %s: fd=%d: convert ipv4 address to ipv6(nat) fail",
                    net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
                return -1;
            }

            connect_rv = net_ev_endpoint_start_connect(driver, endpoint, base_endpoint, remote_addr_ipv6);

            net_address_free(remote_addr_ipv6);
        }
        else {
            connect_rv = net_ev_endpoint_start_connect(driver, endpoint, base_endpoint, remote_addr);
        }
        break;
    case net_local_ip_stack_dual:
        connect_rv = net_ev_endpoint_start_connect(driver, endpoint, base_endpoint, remote_addr);
        break;
    }
    
    if (connect_rv != 0) {
        if (cpe_sock_errno() == EINPROGRESS || cpe_sock_errno() == EWOULDBLOCK) {
            if (net_endpoint_driver_debug(base_endpoint)) {
                net_ev_endpoint_connect_log_connect_start(driver, endpoint, base_endpoint, 1);
            }

            assert(!ev_is_active(&endpoint->m_watcher));
            ev_io_init(
                &endpoint->m_watcher,
                net_ev_endpoint_connect_cb, endpoint->m_fd,
                EV_READ | EV_WRITE);
            ev_io_start(driver->m_ev_loop, &endpoint->m_watcher);
            
            return net_endpoint_set_state(base_endpoint, net_endpoint_state_connecting);
        }
        else {
            net_ev_endpoint_connect_log_connect_error(driver, endpoint, base_endpoint, cpe_sock_errno(), 1);
            net_ev_endpoint_close_sock(driver, endpoint);
            return -1;
        }
    }
    else {
        /*连接成功 */
        if (net_endpoint_driver_debug(base_endpoint)) {
            net_ev_endpoint_connect_log_connect_success(driver, endpoint, base_endpoint);
        }

        if (net_endpoint_address(base_endpoint) == NULL) {
            net_ev_endpoint_update_local_address(endpoint);
        }

        return net_endpoint_set_state(base_endpoint, net_endpoint_state_established);
    }
}

void net_ev_endpoint_close(net_endpoint_t base_endpoint) {
    net_ev_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_ev_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    if (endpoint->m_fd == -1) return;

    ev_io_stop(driver->m_ev_loop, &endpoint->m_watcher);
    net_ev_endpoint_close_sock(driver, endpoint);
}

void net_ev_endpoint_start_rw_watcher(
    net_ev_driver_t driver, net_endpoint_t base_endpoint, net_ev_endpoint_t endpoint)
{
    assert(endpoint->m_fd != -1);

    ev_io_stop(driver->m_ev_loop, &endpoint->m_watcher);
    ev_io_init(
        &endpoint->m_watcher,
        net_ev_endpoint_rw_cb, endpoint->m_fd,
        (net_endpoint_buf_is_full(base_endpoint, net_ep_buf_read) ? 0 : EV_READ)
        | (net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write) ? 0 : EV_WRITE));
    ev_io_start(driver->m_ev_loop, &endpoint->m_watcher);
}

int net_ev_endpoint_update_local_address(net_ev_endpoint_t endpoint) {
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_ev_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(struct sockaddr_storage);
    memset(&addr, 0, addr_len);
    if (cpe_getsockname(endpoint->m_fd, (struct sockaddr *)&addr, &addr_len) != 0) {
        CPE_ERROR(
            driver->m_em, "ev: %s: fd=%d: sockaddr error, errno=%d (%s)",
            net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint),
            endpoint->m_fd, cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        return -1;
    }

    net_address_t address =
        net_address_create_from_sockaddr(net_ev_driver_schedule(driver), (struct sockaddr *)&addr, addr_len);
    if (address == NULL) {
        CPE_ERROR(
            driver->m_em, "ev: %s: fd=%d: create address fail",
            net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
        return -1;
    }

    net_endpoint_set_address(base_endpoint, address, 1);
    
    return 0;
}

int net_ev_endpoint_update_remote_address(net_ev_endpoint_t endpoint) {
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_ev_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(struct sockaddr_storage);
    memset(&addr, 0, addr_len);
    if (cpe_getpeername(endpoint->m_fd, (struct sockaddr *)&addr, &addr_len) != 0) {
        CPE_ERROR(
            driver->m_em, "ev: %s: fd=%d: getpeername error, errno=%d (%s)",
            net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        return -1;
    }

    net_address_t address =
        net_address_create_from_sockaddr(net_ev_driver_schedule(driver), (struct sockaddr *)&addr, addr_len);
    if (address == NULL) {
        CPE_ERROR(
            driver->m_em, "ev: %s: fd=%d: create address fail",
            net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
        return -1;
    }

    net_endpoint_set_remote_address(base_endpoint, address, 1);
    
    return 0;
}

static void net_ev_endpoint_rw_cb(EV_P_ ev_io *w, int revents) {
    net_ev_endpoint_t endpoint = w->data;
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_ev_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    ev_io_stop(driver->m_ev_loop, &endpoint->m_watcher);

    if (revents & EV_READ) {
        for(;net_endpoint_state(base_endpoint) == net_endpoint_state_established;) {
            uint32_t capacity = 0;
            void * rbuf = net_endpoint_buf_alloc(base_endpoint, &capacity);
            if (rbuf == NULL) {
                if (net_endpoint_state(base_endpoint) == net_endpoint_state_deleting) {
                    if (endpoint->m_fd != -1) {
                        net_ev_endpoint_close_sock(driver, endpoint);
                    }
                    return;
                }
                else if (net_endpoint_buf_is_full(base_endpoint, net_ep_buf_read)) {
                    break;
                }
                else {
                    CPE_ERROR(
                        driver->m_em, "ev: %s: fd=%d: alloc rbuf fail, and rbuf is not full!!!",
                        net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
                    if (endpoint->m_fd != -1) {
                        net_ev_endpoint_close_sock(driver, endpoint);
                    }

                    if (net_endpoint_is_active(base_endpoint)) {
                        if (!net_endpoint_have_error(base_endpoint)) {
                            net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_logic);
                        }
                        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_logic_error) != 0) {
                            if (net_endpoint_driver_debug(base_endpoint) >= 2) {
                                CPE_INFO(
                                    driver->m_em, "ev: %s: fd=%d: free for process fail!",
                                    net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
                            }
                            net_endpoint_free(base_endpoint);
                        }
                    }
                }
            }

            assert(endpoint->m_fd != -1);
            int bytes = cpe_recv(endpoint->m_fd, rbuf, capacity, 0);
            if (bytes > 0) {
                if (net_endpoint_driver_debug(base_endpoint)) {
                    CPE_INFO(
                        driver->m_em, "ev: %s: fd=%d: recv %d bytes data!",
                        net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
                        bytes);
                }

                if (driver->m_data_monitor_fun) {
                    driver->m_data_monitor_fun(driver->m_data_monitor_ctx, base_endpoint, net_data_in, (uint32_t)bytes);
                }
                
                if (net_endpoint_buf_supply(base_endpoint, net_ep_buf_read, (uint32_t)bytes) != 0) {
                    if (endpoint->m_fd != -1) {
                        net_ev_endpoint_close_sock(driver, endpoint);
                    }

                    if (net_endpoint_is_active(base_endpoint)) {
                        if (!net_endpoint_have_error(base_endpoint)) {
                            net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_logic);
                        }
                        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_logic_error) != 0) {
                            if (net_endpoint_driver_debug(base_endpoint) >= 2) {
                                CPE_INFO(
                                    driver->m_em, "ev: %s: fd=%d: free for process fail!",
                                    net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
                            }
                            net_endpoint_free(base_endpoint);
                        }
                    }
                    return;
                }
            }
            else if (bytes == 0) {
                net_endpoint_buf_release(base_endpoint);
                
                if (net_endpoint_driver_debug(base_endpoint) >= 2) {
                    CPE_INFO(
                        driver->m_em, "ev: %s: fd=%d: remote disconnected(recv 0)!",
                        net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
                }

                net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_remote_closed);
                if (net_endpoint_set_state(base_endpoint, net_endpoint_state_disable) != 0) {
                    net_endpoint_free(base_endpoint);
                }
                return;
            }
            else {
                assert(bytes == -1);
                net_endpoint_buf_release(base_endpoint);
                
                if (cpe_sock_errno() == EWOULDBLOCK || cpe_sock_errno() == EINPROGRESS) {
                    break;
                }
                else if (cpe_sock_errno() == EINTR) {
                    continue;
                }
                else {
                    CPE_ERROR(
                        driver->m_em, "ev: %s: fd=%d: recv error, errno=%d (%s)!",
                        net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
                        cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));

                    net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_network_error);
					net_ev_endpoint_close_sock(driver, endpoint);
                    if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
                        net_endpoint_free(base_endpoint);
                    }
                    return;
                }
            }
        }
    }

    if (revents & EV_WRITE) {
        while(net_endpoint_state(base_endpoint) == net_endpoint_state_established
              && !net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write))
        {
            uint32_t data_size;
            void * data = net_endpoint_buf_peak(base_endpoint, net_ep_buf_write, &data_size);

            assert(data_size > 0);
            assert(data);
            assert(endpoint->m_fd != -1);

            int bytes = cpe_send(endpoint->m_fd, data, data_size, CPE_SOCKET_DEFAULT_SEND_FLAGS);
            if (bytes > 0) {
                if (net_endpoint_driver_debug(base_endpoint)) {
                    CPE_INFO(
                        driver->m_em, "ev: %s: fd=%d: send %d bytes data!",
                        net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
                        bytes);
                }

                net_endpoint_buf_consume(base_endpoint, net_ep_buf_write,  (uint32_t)bytes);

                if (driver->m_data_monitor_fun) {
                    driver->m_data_monitor_fun(driver->m_data_monitor_ctx, base_endpoint, net_data_out, (uint32_t)bytes);
                }
            }
            else if (bytes == 0) {
                if (net_endpoint_driver_debug(base_endpoint) >= 2) {
                    CPE_INFO(
                        driver->m_em, "ev: %s: fd=%d: free for send return 0!",
                        net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
                }

                net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_remote_closed);
				net_ev_endpoint_close_sock(driver, endpoint);
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

                if (err == EPIPE) {
                    if (net_endpoint_driver_debug(base_endpoint) >= 2) {
                        CPE_INFO(
                            driver->m_em, "ev: %s: fd=%d: free for send recv EPIPE!",
                            net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
                    }

                    net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_network_error);
					net_ev_endpoint_close_sock(driver, endpoint);
                    if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
                        net_endpoint_free(base_endpoint);
                    }
                    return;
                }

                CPE_ERROR(
                    driver->m_em, "ev: %s: fd=%d: free for send error, errno=%d (%s)!",
                    net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
                    cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));

                net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_network_error);
                net_ev_endpoint_close_sock(driver, endpoint);
                if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
                    net_endpoint_free(base_endpoint);
                }
                return;
            }
        }
    }

    if (net_endpoint_state(base_endpoint) == net_endpoint_state_established) {
        net_ev_endpoint_start_rw_watcher(driver, base_endpoint, endpoint);
    }
}

static void net_ev_endpoint_connect_cb(EV_P_ ev_io *w, int revents) {
    net_ev_endpoint_t endpoint = w->data;
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_ev_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    ev_io_stop(driver->m_ev_loop, &endpoint->m_watcher);

    int err = 0;
    socklen_t err_len = sizeof(err);
    if (cpe_getsockopt(endpoint->m_fd, SOL_SOCKET, SO_ERROR, (void*)&err, &err_len) == -1) {
        CPE_ERROR(
            driver->m_em, "ev: %s: fd=%d: connect_cb get socket error fail, errno=%d (%s)!",
            net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));

        net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_network_error);
        net_ev_endpoint_close_sock(driver, endpoint);
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
            net_endpoint_free(base_endpoint);
        }
        return;
    }

    if (err != 0) {
        if (err == EINPROGRESS || err == EWOULDBLOCK) {
            if (net_endpoint_driver_debug(base_endpoint)) {
                net_ev_endpoint_connect_log_connect_start(driver, endpoint, base_endpoint, 1);
            }

            assert(!ev_is_active(&endpoint->m_watcher));
            ev_io_init(
                &endpoint->m_watcher,
                net_ev_endpoint_connect_cb, endpoint->m_fd,
                EV_READ | EV_WRITE);
            ev_io_start(driver->m_ev_loop, &endpoint->m_watcher);
        }
        else {
            net_ev_endpoint_connect_log_connect_error(driver, endpoint, base_endpoint, err, 0);

            net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_network_error);
            net_ev_endpoint_close_sock(driver, endpoint);
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
                net_endpoint_free(base_endpoint);
            }
        }
        return;
    }

    if (net_endpoint_driver_debug(base_endpoint)) {
        net_ev_endpoint_connect_log_connect_success(driver, endpoint, base_endpoint);
    }

    if (net_endpoint_address(base_endpoint) == NULL) {
        net_ev_endpoint_update_local_address(endpoint);
    }

    net_ev_endpoint_start_rw_watcher(driver, base_endpoint, endpoint);
    if (net_endpoint_set_state(base_endpoint, net_endpoint_state_established) != 0) {
        net_endpoint_free(base_endpoint);
        return;
    }
}

static void net_ev_endpoint_connect_log_connect_start(
    net_ev_driver_t driver, net_ev_endpoint_t endpoint, net_endpoint_t base_endpoint, uint8_t is_first)
{
    net_address_t local_address = net_endpoint_address(base_endpoint);
    if (local_address) {
        char local_addr_buf[128];
        cpe_str_dup(
            local_addr_buf, sizeof(local_addr_buf),
            net_address_dump(net_ev_driver_tmp_buffer(driver), local_address));

        CPE_INFO(
            driver->m_em, "ev: %s: fd=%d: connect %s (local-address=%s)",
            net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            local_addr_buf,
            is_first ? "start" : "restart");
    }
    else {
        CPE_INFO(
            driver->m_em, "ev: %s: fd=%d: connect %s",
            net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            is_first ? "start" : "restart");
    }
}

static void net_ev_endpoint_connect_log_connect_success(
    net_ev_driver_t driver, net_ev_endpoint_t endpoint, net_endpoint_t base_endpoint)
{
    net_address_t local_address = net_endpoint_address(base_endpoint);
    if (local_address) {
        char local_addr_buf[128];
        cpe_str_dup(
            local_addr_buf, sizeof(local_addr_buf),
            net_address_dump(net_ev_driver_tmp_buffer(driver), local_address));

        CPE_INFO(
            driver->m_em, "ev: %s: fd=%d: connect success (local-address=%s)",
            net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            local_addr_buf);
    }
    else {
        CPE_INFO(
            driver->m_em, "ev: %s: fd=%d: connect success",
            net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
    }
}

static void net_ev_endpoint_connect_log_connect_error(
    net_ev_driver_t driver, net_ev_endpoint_t endpoint, net_endpoint_t base_endpoint, int err, uint8_t is_first)
{
    CPE_ERROR(
        driver->m_em, "ev: %s: fd=%d: connect error%s, errno=%d (%s)",
        net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
        is_first ? "" : "(callback)",
        err, cpe_sock_errstr(err));
}

static int net_ev_endpoint_start_connect(
    net_ev_driver_t driver, net_ev_endpoint_t endpoint, net_endpoint_t base_endpoint, net_address_t remote_addr)
{
    struct sockaddr_storage remote_addr_sock;
    socklen_t remote_addr_sock_len = sizeof(remote_addr_sock);
    net_address_to_sockaddr(remote_addr, (struct sockaddr *)&remote_addr_sock, &remote_addr_sock_len);

    assert(net_address_type(remote_addr) == net_address_ipv4 || net_address_type(remote_addr) == net_address_ipv6);
    
    int domain = net_address_type(remote_addr) == net_address_ipv4 ? AF_INET : AF_INET6; 

    endpoint->m_fd = cpe_sock_open(domain, SOCK_STREAM, IPPROTO_TCP);
    if (endpoint->m_fd == -1) {
        CPE_ERROR(
            driver->m_em, "ev: %s: fd=%d: create socket fail, errno=%d (%s)",
            net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        return -1;
    }

    net_address_t local_address = net_endpoint_address(base_endpoint);
    if (local_address) {
        struct sockaddr_storage local_addr_sock;
        socklen_t local_addr_sock_len;
        bzero(&local_addr_sock, sizeof(local_addr_sock));
        local_addr_sock_len = sizeof(local_addr_sock);

        if (net_address_to_sockaddr(local_address, (struct sockaddr *)&local_addr_sock, &local_addr_sock_len) != 0) {
            CPE_ERROR(
                driver->m_em, "ev: %s: fd=%d: connect not support connect to domain address!",
                net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
            return -1;
        }

        if (cpe_sock_set_reuseaddr(endpoint->m_fd, 1) != 0) {
            CPE_ERROR(
                driver->m_em, "ev: %s: fd=%d: set sock reuse address fail, errno=%d (%s)",
                net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
            net_ev_endpoint_close_sock(driver, endpoint);
            return -1;
        }

        if(cpe_bind(endpoint->m_fd, (struct sockaddr *)&local_addr_sock, local_addr_sock_len) != 0) {
            char local_addr_buf[128];
            cpe_str_dup(
                local_addr_buf, sizeof(local_addr_buf),
                net_address_dump(net_ev_driver_tmp_buffer(driver), local_address));

            CPE_ERROR(
                driver->m_em, "ev: %s: fd=%d: bind local address %s fail, errno=%d (%s)",
                net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd, local_addr_buf,
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
            return -1;
        }
    }

    if (cpe_sock_set_none_block(endpoint->m_fd, 1) != 0) {
        CPE_ERROR(
            driver->m_em, "ev: %s: fd=%d: set non-block fail, errno=%d (%s)",
            net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        net_ev_endpoint_close_sock(driver, endpoint);
        return -1;
    }

    if (cpe_sock_set_no_sigpipe(endpoint->m_fd, 1) != 0) {
        CPE_ERROR(
            driver->m_em, "ev: %s: fd=%d: set no-sig-pipe fail, errno=%d (%s)",
            net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        net_ev_endpoint_close_sock(driver, endpoint);
        return -1;
    }

    /* char buf[128]; */
    /* cpe_str_dup(buf, sizeof(buf), net_address_dump(net_ev_driver_tmp_buffer(driver), remote_addr)); */
    
    /* CPE_ERROR( */
    /*     driver->m_em, "ev: %s: fd=%d: connect to %s", */
    /*     net_endpoint_dump(net_ev_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,*/
    /*     buf); */
    
    return cpe_connect(endpoint->m_fd, (struct sockaddr *)&remote_addr_sock, remote_addr_sock_len);
}

#if WIN32
extern void ev_changes_remove(EV_P_ int fd);
#endif

static void net_ev_endpoint_close_sock(net_ev_driver_t driver, net_ev_endpoint_t endpoint) {
    //net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);

    assert(endpoint->m_fd != -1);

#if WIN32
    ev_changes_remove(driver->m_ev_loop, endpoint->m_fd);
#endif

    cpe_sock_close(endpoint->m_fd);
    endpoint->m_fd = -1;
}
