#include "assert.h"
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils_sock/sock_utils.h"
#include "net_endpoint.h"
#include "net_address.h"
#include "net_driver.h"
#include "net_watcher.h"
#include "net_sock_endpoint_i.h"

static void net_sock_endpoint_rw_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write);
static void net_sock_endpoint_connect_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write);

static uint8_t net_sock_endpoint_on_read(net_sock_driver_t driver, net_sock_endpoint_t endpoint, net_endpoint_t base_endpoint);
static uint8_t net_sock_endpoint_on_write(net_sock_driver_t driver, net_sock_endpoint_t endpoint, net_endpoint_t base_endpoint);

static int net_sock_endpoint_start_connect(
    net_sock_driver_t driver, net_sock_endpoint_t endpoint, net_endpoint_t base_endpoint, net_address_t remote_address);

static void net_sock_endpoint_connect_log_connect_start(
    net_sock_driver_t driver, net_sock_endpoint_t endpoint, net_endpoint_t base_endpoint, uint8_t is_first);
static void net_sock_endpoint_connect_log_connect_success(
    net_sock_driver_t driver, net_sock_endpoint_t endpoint, net_endpoint_t base_endpoint);

static void net_sock_endpoint_close_sock(net_sock_driver_t driver, net_sock_endpoint_t endpoint);

int net_sock_endpoint_init(net_endpoint_t base_endpoint) {
    net_sock_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    
    endpoint->m_fd = -1;
    endpoint->m_watcher = NULL;
    endpoint->m_local_address_auto = NULL;
    
    return 0;
}

void net_sock_endpoint_fini(net_endpoint_t base_endpoint) {
    net_sock_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_sock_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    net_sock_endpoint_close_sock(driver, endpoint);

    assert(endpoint->m_fd == -1);
    assert(endpoint->m_watcher == NULL);

    if (endpoint->m_local_address_auto) {
        net_address_free(endpoint->m_local_address_auto);
        endpoint->m_local_address_auto = NULL;
    }
}

int net_sock_endpoint_update(net_endpoint_t base_endpoint) {
    net_sock_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_sock_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    assert(net_endpoint_state(base_endpoint) == net_endpoint_state_established);
    assert(endpoint->m_watcher != NULL);

    if (!net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write) /*有数据等待写入 */
        && !net_watcher_expect_write(endpoint->m_watcher)) /*socket没有等待可以写入的操作（当前可以写入数据到socket) */
    {
        if (net_endpoint_driver_debug(base_endpoint) >= 3) {
            CPE_INFO(
                driver->m_em, "sock: %s: fd=%d: write begin!",
                net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
        }

        net_watcher_update_write(endpoint->m_watcher, 1);
        net_endpoint_set_is_writing(base_endpoint, 1);
        if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return 0;
    }

    assert(net_endpoint_state(base_endpoint) == net_endpoint_state_established);
    assert(endpoint->m_watcher != NULL);

    if (net_endpoint_expect_read(base_endpoint)) {
        if (!net_watcher_expect_read(endpoint->m_watcher)) { /*socket上没有等待读取的操作（当前有数据可以读取) */
            if (net_endpoint_driver_debug(base_endpoint) >= 3) {
                CPE_INFO(
                    driver->m_em, "sock: %s: fd=%d: read begin!",
                    net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
            }

            net_watcher_update_read(endpoint->m_watcher, 1);
        }
    }
    else {
        if (net_watcher_expect_read(endpoint->m_watcher)) {
            if (net_endpoint_driver_debug(base_endpoint) >= 3) {
                CPE_INFO(
                    driver->m_em, "sock: %s: fd=%d: read stop!",
                    net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
            }

            net_watcher_update_read(endpoint->m_watcher, 0);
        }
    }
    
    return 0;
}

int net_sock_endpoint_connect(net_endpoint_t base_endpoint) {
    net_driver_t base_driver = net_endpoint_driver(base_endpoint);
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);
    net_sock_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_sock_driver_t driver = net_driver_data(base_driver);

CONNECT_AGAIN:    
    if (endpoint->m_fd != -1) {
        CPE_ERROR(
            driver->m_em, "sock: %s: fd=%d: already connected!",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
        return -1;
    }

    net_address_t remote_addr = net_endpoint_remote_address(base_endpoint);
    if (remote_addr == NULL) {
        CPE_ERROR(
            driver->m_em, "sock: %s: fd=%d: connect with no remote address!",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
        return -1;
    }
    remote_addr = net_address_resolved(remote_addr);

    if (net_address_type(remote_addr) == net_address_domain) {
        CPE_ERROR(
            driver->m_em, "sock: %s: fd=%d: connect not support domain address!",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
        return -1;
    }
    
    int connect_rv;

    if (net_address_type(remote_addr) == net_address_local) {
        connect_rv = net_sock_endpoint_start_connect(driver, endpoint, base_endpoint, remote_addr);
    }
    else {
        assert(net_address_type(remote_addr) == net_address_ipv4 || net_address_type(remote_addr) == net_address_ipv6);

        net_local_ip_stack_t ip_stack = net_schedule_local_ip_stack(schedule);
        switch(ip_stack) {
        case net_local_ip_stack_none:
            CPE_ERROR(
                driver->m_em, "sock: %s: fd=%d: can`t connect in %s!",
                net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
                net_local_ip_stack_str(ip_stack));
            return -1;
        case net_local_ip_stack_ipv4:
            if (net_address_type(remote_addr) == net_address_ipv6) {
                if (net_address_ipv6_is_ipv4_map(remote_addr)) {
                    net_address_t remote_addr_ipv4 = net_address_create_ipv4_from_ipv6_map(net_endpoint_schedule(base_endpoint), remote_addr);
                    if (remote_addr_ipv4 == NULL) {
                        CPE_ERROR(
                            driver->m_em, "sock: %s: fd=%d: convert ipv6 address to ipv4(map) fail",
                            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
                        return -1;
                    }

                    connect_rv = net_sock_endpoint_start_connect(driver, endpoint, base_endpoint, remote_addr_ipv4);

                    net_address_free(remote_addr_ipv4);
                }
                else {
                    CPE_ERROR(
                        driver->m_em, "sock: %s: fd=%d: can`t connect to ipv6 network in ipv4 network env!",
                        net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
                    return -1;
                }
            }
            else {
                connect_rv = net_sock_endpoint_start_connect(driver, endpoint, base_endpoint, remote_addr);
            }
            break;
        case net_local_ip_stack_ipv6:
            if (net_address_type(remote_addr) == net_address_ipv4) {
                net_address_t remote_addr_ipv6 = net_address_create_ipv6_from_ipv4_nat(net_endpoint_schedule(base_endpoint), remote_addr);
                if (remote_addr_ipv6 == NULL) {
                    CPE_ERROR(
                        driver->m_em, "sock: %s: fd=%d: convert ipv4 address to ipv6(nat) fail",
                        net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
                    return -1;
                }

                connect_rv = net_sock_endpoint_start_connect(driver, endpoint, base_endpoint, remote_addr_ipv6);

                net_address_free(remote_addr_ipv6);
            }
            else {
                connect_rv = net_sock_endpoint_start_connect(driver, endpoint, base_endpoint, remote_addr);
            }
            break;
        case net_local_ip_stack_dual:
            connect_rv = net_sock_endpoint_start_connect(driver, endpoint, base_endpoint, remote_addr);
            break;
        }
    }

    if (connect_rv != 0) {
        if (endpoint->m_fd == -1) {
            return -1;
        }
        
        if (cpe_sock_errno() == EINPROGRESS || cpe_sock_errno() == EWOULDBLOCK) {
            if (net_endpoint_driver_debug(base_endpoint)) {
                net_sock_endpoint_connect_log_connect_start(driver, endpoint, base_endpoint, 1);
            }

            if (net_endpoint_address(base_endpoint) == NULL) {
                net_sock_endpoint_update_local_address(endpoint);
            }

            assert(endpoint->m_watcher == NULL);
            endpoint->m_watcher = net_watcher_create(base_driver, _to_watcher_fd(endpoint->m_fd), endpoint, net_sock_endpoint_connect_cb);
            if (endpoint->m_watcher == NULL) {
                CPE_ERROR(
                    driver->m_em, "sock: %s: fd=%d: create watcher fail",
                    net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
                
                if (net_endpoint_set_state(base_endpoint, net_endpoint_state_logic_error) != 0) {
                    if (net_endpoint_driver_debug(base_endpoint) >= 2) {
                        CPE_INFO(
                            driver->m_em, "sock: %s: fd=%d: free for process fail!",
                            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
                    }
                    net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
                    return 0;
                }

                return -1;
            }

            net_watcher_update(endpoint->m_watcher, 1, 1);
            return net_endpoint_set_state(base_endpoint, net_endpoint_state_connecting);
        }
        else {
            CPE_ERROR(
                driver->m_em, "sock: %s: fd=%d: connect error(first), errno=%d (%s)",
                net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));

            net_sock_endpoint_close_sock(driver, endpoint);

            if (net_endpoint_shift_address(base_endpoint)) {
                goto CONNECT_AGAIN;
            }
            else {
                net_endpoint_set_error(
                    base_endpoint, net_endpoint_error_source_network,
                    net_endpoint_network_errno_connect_error, cpe_sock_errstr(cpe_sock_errno()));

                if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) return -1;
                return -1;
            }
        }
    }
    else {
        /*连接成功 */
        if (net_endpoint_driver_debug(base_endpoint)) {
            net_sock_endpoint_connect_log_connect_success(driver, endpoint, base_endpoint);
        }

        if (net_endpoint_address(base_endpoint) == NULL) {
            net_sock_endpoint_update_local_address(endpoint);
        }

        if (net_sock_endpoint_set_established(driver, endpoint, base_endpoint) != 0) {
            net_sock_endpoint_close_sock(driver, endpoint);
            return -1;
        }

        return 0;
    }
}

void net_sock_endpoint_close(net_endpoint_t base_endpoint) {
    net_sock_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_sock_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    if (endpoint->m_fd == -1) return;

    net_sock_endpoint_close_sock(driver, endpoint);
}

int net_sock_endpoint_set_established(net_sock_driver_t driver, net_sock_endpoint_t endpoint, net_endpoint_t base_endpoint) {
    assert(endpoint->m_watcher == NULL);
    assert(endpoint->m_fd != -1);

    endpoint->m_watcher = net_watcher_create(net_endpoint_driver(base_endpoint), _to_watcher_fd(endpoint->m_fd), endpoint, net_sock_endpoint_rw_cb);
    if (endpoint->m_watcher == NULL) {
        CPE_ERROR(
            driver->m_em, "sock: %s: fd=%d: set established: create watcher fail",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
        return -1;
    }
        
    if (net_endpoint_set_state(base_endpoint, net_endpoint_state_established) != 0) return -1;

    return net_sock_endpoint_update(base_endpoint);
}

int net_sock_endpoint_update_local_address(net_sock_endpoint_t endpoint) {
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_sock_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(struct sockaddr_storage);
    memset(&addr, 0, addr_len);
    if (cpe_getsockname(endpoint->m_fd, (struct sockaddr *)&addr, &addr_len) != 0) {
        CPE_ERROR(
            driver->m_em, "sock: %s: fd=%d: sockaddr error, errno=%d (%s)",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint),
            endpoint->m_fd, cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        return -1;
    }

    net_address_t address =
        net_address_create_from_sockaddr(net_sock_driver_schedule(driver), (struct sockaddr *)&addr, addr_len);
    if (address == NULL) {
        CPE_ERROR(
            driver->m_em, "sock: %s: fd=%d: create address fail",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
        return -1;
    }

    if (net_endpoint_set_address(base_endpoint, address, 0) != 0) {
        CPE_ERROR(
            driver->m_em, "sock: %s: fd=%d: create local address fail",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
        net_address_free(address);
        return -1;
    }

    if (endpoint->m_local_address_auto) {
        net_address_free(endpoint->m_local_address_auto);
    }
    endpoint->m_local_address_auto = address;
    
    return 0;
}

int net_sock_endpoint_update_remote_address(net_sock_endpoint_t endpoint) {
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_sock_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(struct sockaddr_storage);
    memset(&addr, 0, addr_len);
    if (cpe_getpeername(endpoint->m_fd, (struct sockaddr *)&addr, &addr_len) != 0) {
        CPE_ERROR(
            driver->m_em, "sock: %s: fd=%d: getpeername error, errno=%d (%s)",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        return -1;
    }

    net_address_t address =
        net_address_create_from_sockaddr(net_sock_driver_schedule(driver), (struct sockaddr *)&addr, addr_len);
    if (address == NULL) {
        CPE_ERROR(
            driver->m_em, "sock: %s: fd=%d: create address fail",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
        return -1;
    }

    net_endpoint_set_remote_address(base_endpoint, address, 1);
    
    return 0;
}

static uint8_t net_sock_endpoint_on_read(net_sock_driver_t driver, net_sock_endpoint_t endpoint, net_endpoint_t base_endpoint) {
    while(net_endpoint_state(base_endpoint) == net_endpoint_state_established  /*当前状态正确 */
          && net_endpoint_expect_read(base_endpoint))
    {
        uint32_t capacity = 0;
        void * rbuf = net_endpoint_buf_alloc(base_endpoint, &capacity);
        if (rbuf == NULL) {
            net_sock_endpoint_close_sock(driver, endpoint);
            
            if (net_endpoint_state(base_endpoint) == net_endpoint_state_deleting) {
                return -1;
            }
            else {
                CPE_ERROR(
                    driver->m_em, "sock: %s: fd=%d: on read: endpoint rbuf full!",
                    net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);

                if (net_endpoint_is_active(base_endpoint)) {
                    if (!net_endpoint_have_error(base_endpoint)) {
                        net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_logic, NULL);
                    }
                
                    if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) return -1;
                }

                return 0;
            }
        }

        if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) break;
        
        assert(endpoint->m_fd != -1);
        ssize_t bytes = cpe_recv(endpoint->m_fd, rbuf, capacity, 0);
        if (bytes > 0) {
            if (net_endpoint_driver_debug(base_endpoint)) {
                CPE_INFO(
                    driver->m_em, "sock: %s: fd=%d: recv %d bytes data!",
                    net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
                    (int)bytes);
            }

            if (net_endpoint_buf_supply(base_endpoint, net_ep_buf_read, (uint32_t)bytes) != 0) {
                net_sock_endpoint_close_sock(driver, endpoint);
                
                if (net_endpoint_is_active(base_endpoint)) {
                    if (!net_endpoint_have_error(base_endpoint)) {
                        net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_logic, NULL);
                    }
                    if (net_endpoint_set_state(base_endpoint, net_endpoint_state_logic_error) != 0) {
                        if (net_endpoint_driver_debug(base_endpoint)) {
                            CPE_INFO(
                                driver->m_em, "sock: %s: fd=%d: free for process fail!",
                                net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
                        }
                        return -1;
                    }
                }
            }

            continue;
        }
        else if (bytes == 0) {
            net_endpoint_buf_release(base_endpoint);
            if (net_endpoint_driver_debug(base_endpoint)) {
                CPE_INFO(
                    driver->m_em, "sock: %s: fd=%d: remote disconnected(recv 0)!",
                    net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
            }
                
            net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_remote_closed, NULL);
            net_sock_endpoint_close_sock(driver, endpoint);
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_disable) != 0) return -1;
            return 0;
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
                    driver->m_em, "sock: %s: fd=%d: recv error, errno=%d (%s)!",
                    net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
                    cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));

                net_endpoint_set_error(
                    base_endpoint, net_endpoint_error_source_network,
                    net_endpoint_network_errno_network_error, cpe_sock_errstr(cpe_sock_errno()));

                net_sock_endpoint_close_sock(driver, endpoint);
                if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) return -1;
                return 0;
            }
        }
        assert(0);
    }

    return 0;
}

static uint8_t net_sock_endpoint_on_write(net_sock_driver_t driver, net_sock_endpoint_t endpoint, net_endpoint_t base_endpoint) {
    while(net_endpoint_state(base_endpoint) == net_endpoint_state_established /*ep状态正确 */
          && !net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write) /*还有数据等待写入 */
        )
    {
        uint32_t data_size = 0;
        if (net_endpoint_dft_block_size(base_endpoint) == 0) {
            data_size = net_endpoint_buf_size(base_endpoint, net_ep_buf_write);
        }
        void * data = net_endpoint_buf_peak(base_endpoint, net_ep_buf_write, &data_size);
        assert(data_size > 0);
        assert(data);

        ssize_t bytes = cpe_send(endpoint->m_fd, data, data_size, CPE_SOCKET_DEFAULT_SEND_FLAGS);
        if (bytes > 0) {
            if (net_endpoint_driver_debug(base_endpoint)) {
                CPE_INFO(
                    driver->m_em, "sock: %s: fd=%d: send %d/%d bytes data!",
                    net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
                    (int)bytes, data_size);
            }

            net_endpoint_buf_consume(base_endpoint, net_ep_buf_write, (uint32_t)bytes);
        }
        else if (bytes == 0) {
            if (net_endpoint_driver_debug(base_endpoint)) {
                CPE_INFO(
                    driver->m_em, "sock: %s: fd=%d: free for send return 0!",
                    net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
            }

            net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_remote_closed, NULL);
            net_sock_endpoint_close_sock(driver, endpoint);
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) return -1;
            return 0;
        }
        else {
            int err = cpe_sock_errno();
            assert(bytes == -1);

            if (err == EWOULDBLOCK || err == EINPROGRESS) {
                assert(net_watcher_expect_write(endpoint->m_watcher));
                return 0;
            }
        
            if (err == EINTR) continue;

            if (err == EPIPE) {
                if (net_endpoint_driver_debug(base_endpoint)) {
                    CPE_INFO(
                        driver->m_em, "sock: %s: fd=%d: free for send recv EPIPE!",
                        net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
                }

                net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_network_error, cpe_sock_errstr(err));
            }
            else {
                CPE_ERROR(
                    driver->m_em, "sock: %s: fd=%d: send error, errno=%d (%s)!",
                    net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
                    err, cpe_sock_errstr(err));

                net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_network_error, cpe_sock_errstr(err));
            }

            net_sock_endpoint_close_sock(driver, endpoint);
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) return -1;

            return 0;
        }
    }

    if (net_endpoint_state(base_endpoint) == net_endpoint_state_established) {
        assert(net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write));

        if (net_watcher_expect_write(endpoint->m_watcher)) {
            if (net_endpoint_driver_debug(base_endpoint) >= 3) {
                CPE_INFO(
                    driver->m_em, "sock: %s: fd=%d: write done",
                    net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
            }

            net_watcher_update_write(endpoint->m_watcher, 0);
            net_endpoint_set_is_writing(base_endpoint, 0);
        }
    }

    return 0;
}

static void net_sock_endpoint_rw_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write) {
    net_sock_endpoint_t endpoint = ctx;
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_sock_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    assert(endpoint->m_watcher);
    assert(net_endpoint_state(base_endpoint) == net_endpoint_state_established);
    
    if (do_read) {
        if (net_sock_endpoint_on_read(driver, endpoint, base_endpoint) != 0) {
            net_endpoint_free(base_endpoint);
            return;
        }

        if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return;
    }

    if (do_write) {
        if (net_sock_endpoint_on_write(driver, endpoint, base_endpoint) != 0) {
            net_endpoint_free(base_endpoint);
            return;
        }
    }
}

static void net_sock_endpoint_connect_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write) {
    net_sock_endpoint_t endpoint = ctx;
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_sock_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    assert(fd == endpoint->m_fd);

    int err = 0;
    socklen_t err_len = sizeof(err);
    if (cpe_getsockopt(endpoint->m_fd, SOL_SOCKET, SO_ERROR, (void*)&err, &err_len) == -1) {
        CPE_ERROR(
            driver->m_em, "sock: %s: fd=%d: connect_cb get socket error fail, errno=%d (%s)!",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));

        net_endpoint_set_error(
            base_endpoint, net_endpoint_error_source_network,
            net_endpoint_network_errno_network_error,
            cpe_sock_errstr(cpe_sock_errno()));
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
            net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
            return;
        }

        net_sock_endpoint_close_sock(driver, endpoint);
        return;
    }

    if (err != 0) {
        if (err == EINPROGRESS || err == EWOULDBLOCK) {
            if (net_endpoint_driver_debug(base_endpoint)) {
                net_sock_endpoint_connect_log_connect_start(driver, endpoint, base_endpoint, 1);
            }

            assert(endpoint->m_watcher);
            assert(net_watcher_expect_read(endpoint->m_watcher) && net_watcher_expect_write(endpoint->m_watcher));
            /* assert(sock_is_active(&endpoint->m_watcher)); */
            /* assert(endpoint->m_expects == (EV_READ | EV_WRITE)); */
        }
        else {
            CPE_ERROR(
                driver->m_em, "sock: %s: fd=%d: connect error(callback), errno=%d (%s)",
                net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
                err, cpe_sock_errstr(err));
            net_sock_endpoint_close_sock(driver, endpoint);
            
            if (net_endpoint_shift_address(base_endpoint)) {
                if (net_sock_endpoint_connect(base_endpoint) != 0) {
                    if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
                        net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
                        return;
                    }
                }
            }
            else {
                net_endpoint_set_error(
                    base_endpoint, net_endpoint_error_source_network,
                    net_endpoint_network_errno_connect_error,
                    cpe_sock_errstr(err));
                if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
                    net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
                    return;
                }
            }
        }
        return;
    }

    if (net_endpoint_driver_debug(base_endpoint)) {
        net_sock_endpoint_connect_log_connect_success(driver, endpoint, base_endpoint);
    }

    net_watcher_free(endpoint->m_watcher);
    endpoint->m_watcher = NULL;

    if (net_sock_endpoint_set_established(driver, endpoint, base_endpoint) != 0) {
        net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
        return;
    }
}

static void net_sock_endpoint_connect_log_connect_start(
    net_sock_driver_t driver, net_sock_endpoint_t endpoint, net_endpoint_t base_endpoint, uint8_t is_first)
{
    net_address_t local_address = net_endpoint_address(base_endpoint);
    if (local_address) {
        char local_addr_buf[128];
        cpe_str_dup(
            local_addr_buf, sizeof(local_addr_buf),
            net_address_dump(net_sock_driver_tmp_buffer(driver), local_address));

        CPE_INFO(
            driver->m_em, "sock: %s: fd=%d: connect %s (local-address=%s)",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            local_addr_buf,
            is_first ? "start" : "restart");
    }
    else {
        CPE_INFO(
            driver->m_em, "sock: %s: fd=%d: connect %s",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            is_first ? "start" : "restart");
    }
}

static void net_sock_endpoint_connect_log_connect_success(
    net_sock_driver_t driver, net_sock_endpoint_t endpoint, net_endpoint_t base_endpoint)
{
    net_address_t local_address = net_endpoint_address(base_endpoint);
    if (local_address) {
        char local_addr_buf[128];
        cpe_str_dup(
            local_addr_buf, sizeof(local_addr_buf),
            net_address_dump(net_sock_driver_tmp_buffer(driver), local_address));

        CPE_INFO(
            driver->m_em, "sock: %s: fd=%d: connect success (local-address=%s)",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            local_addr_buf);
    }
    else {
        CPE_INFO(
            driver->m_em, "sock: %s: fd=%d: connect success",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
    }
}

static int net_sock_endpoint_start_connect(
    net_sock_driver_t driver, net_sock_endpoint_t endpoint, net_endpoint_t base_endpoint, net_address_t remote_addr)
{
    struct sockaddr_storage remote_addr_sock;
    socklen_t remote_addr_sock_len = sizeof(remote_addr_sock);
    net_address_to_sockaddr(remote_addr, (struct sockaddr *)&remote_addr_sock, &remote_addr_sock_len);

    int domain;
    int protocol;
    switch(net_address_type(remote_addr)) {
    case net_address_ipv4:
        domain = AF_INET;
        protocol = IPPROTO_TCP;
        break;
    case net_address_ipv6:
        domain = AF_INET6;
        protocol = IPPROTO_TCP;
        break;
    case net_address_local:
#if _MSC_VER || __MINGW32__
        CPE_ERROR(
            driver->m_em, "sock: %s: fd=%d: connect win32 not support AF_LOCAL",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint),
            endpoint->m_fd);
        return -1;
#else        
        domain = AF_LOCAL;
        protocol = 0;
#endif        
        break;
    case net_address_domain:
        CPE_ERROR(
            driver->m_em, "sock: %s: fd=%d: connect not support %s",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint),
            endpoint->m_fd,
            net_address_type_str(net_address_type(remote_addr)));
        return -1;
    }

    endpoint->m_fd = cpe_sock_open(domain, SOCK_STREAM, protocol);
    if (endpoint->m_fd == -1) {
        CPE_ERROR(
            driver->m_em, "sock: %s: fd=%d: create socket fail, errno=%d (%s)",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        return -1;
    }

    net_address_t local_address = net_endpoint_address(base_endpoint);

    if (endpoint->m_local_address_auto) {
        uint8_t is_local_address_auto = net_address_cmp(endpoint->m_local_address_auto, local_address) == 0 ? 1 : 0;

        net_address_free(endpoint->m_local_address_auto);
        endpoint->m_local_address_auto = NULL;

        if (is_local_address_auto) {
            net_endpoint_set_address(base_endpoint, NULL, 0);
            local_address = NULL;
        }
    }
    
    if (local_address) {
        struct sockaddr_storage local_addr_sock;
        socklen_t local_addr_sock_len;
        bzero(&local_addr_sock, sizeof(local_addr_sock));
        local_addr_sock_len = sizeof(local_addr_sock);

        if (net_address_to_sockaddr(local_address, (struct sockaddr *)&local_addr_sock, &local_addr_sock_len) != 0) {
            CPE_ERROR(
                driver->m_em, "sock: %s: fd=%d: connect not support connect to domain address!",
                net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd);
            net_sock_endpoint_close_sock(driver, endpoint);
            return -1;
        }

        if (cpe_sock_set_reuseaddr(endpoint->m_fd, 1) != 0) {
            CPE_ERROR(
                driver->m_em, "sock: %s: fd=%d: set sock reuse address fail, errno=%d (%s)",
                net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
            net_sock_endpoint_close_sock(driver, endpoint);
            return -1;
        }

        if(cpe_bind(endpoint->m_fd, (struct sockaddr *)&local_addr_sock, local_addr_sock_len) != 0) {
            char local_addr_buf[128];
            cpe_str_dup(
                local_addr_buf, sizeof(local_addr_buf),
                net_address_dump(net_sock_driver_tmp_buffer(driver), local_address));

            CPE_ERROR(
                driver->m_em, "sock: %s: fd=%d: bind local address %s fail, errno=%d (%s)",
                net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd, local_addr_buf,
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
            net_sock_endpoint_close_sock(driver, endpoint);
            return -1;
        }
    }

    if (cpe_sock_set_none_block(endpoint->m_fd, 1) != 0) {
        CPE_ERROR(
            driver->m_em, "sock: %s: fd=%d: set non-block fail, errno=%d (%s)",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        net_sock_endpoint_close_sock(driver, endpoint);
        return -1;
    }

    if (cpe_sock_set_no_sigpipe(endpoint->m_fd, 1) != 0) {
        CPE_ERROR(
            driver->m_em, "sock: %s: fd=%d: set no-sig-pipe fail, errno=%d (%s)",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        net_sock_endpoint_close_sock(driver, endpoint);
        return -1;
    }

    if (cpe_sock_set_send_timeout(endpoint->m_fd, 30 * 1000) != 0) {
        CPE_ERROR(
            driver->m_em, "sock: %s: fd=%d: set send timeout fail, errno=%d (%s)",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        net_sock_endpoint_close_sock(driver, endpoint);
        return -1;
    }

    if (cpe_sock_set_recv_timeout(endpoint->m_fd, 30 * 1000) != 0) {
        CPE_ERROR(
            driver->m_em, "sock: %s: fd=%d: set recv timeout fail, errno=%d (%s)",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        net_sock_endpoint_close_sock(driver, endpoint);
        return -1;
    }

    return cpe_connect(endpoint->m_fd, (struct sockaddr *)&remote_addr_sock, remote_addr_sock_len);
}

static void net_sock_endpoint_close_sock(net_sock_driver_t driver, net_sock_endpoint_t endpoint) {
    if (endpoint->m_watcher) {
        net_watcher_free(endpoint->m_watcher);
        endpoint->m_watcher = NULL;
    }

    if (endpoint->m_fd != -1) {
        cpe_sock_close(endpoint->m_fd);
        endpoint->m_fd = -1;
    }
}

net_sock_endpoint_t net_sock_endpoint_from_base_endpoint(net_endpoint_t ep) {
    if (net_driver_init_fun(net_endpoint_driver(ep)) == net_sock_driver_init) {
        return net_endpoint_data(ep);
    }
    else {
        return NULL;
    }
}

net_endpoint_t net_sock_endpoint_base_endpoint(net_sock_endpoint_t endpoint) {
    return net_endpoint_from_data(endpoint);
}

int net_sock_endpoint_fd(net_sock_endpoint_t endpoint) {
    return endpoint->m_fd;
}

int net_sock_endpoint_set_dft_block_size_to_mss(net_sock_endpoint_t endpoint) {
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_sock_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    
    int mss = cpe_sock_get_tcp_mss(endpoint->m_fd);
    if (mss < 0) {
        CPE_ERROR(
            driver->m_em, "sock: %s: fd=%d: get mss fail, errno=%d (%s)",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        assert(0);
        return -1;
    }
    else {
        if (net_endpoint_driver_debug(base_endpoint)) {
            CPE_INFO(
                driver->m_em, "sock: %s: fd=%d: dft-block-size to mss %d",
                net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd, mss);
        }

        net_endpoint_set_dft_block_size(base_endpoint, (uint32_t)mss);
        return 0;
    }
}

int net_sock_endpoint_set_no_delay(net_sock_endpoint_t endpoint, uint8_t is_enable) {
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_sock_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    if (cpe_sock_set_no_delay(endpoint->m_fd, is_enable) != 0) {
        CPE_ERROR(
            driver->m_em, "sock: %s: fd=%d: set sock nodelay to %s error, errno=%d (%s)!",
            net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint),
            endpoint->m_fd, is_enable ? "enable" : "disable",
            errno, strerror(errno));
        return -1;
    } else {
        if (net_endpoint_driver_debug(base_endpoint) >= 2) {
            CPE_INFO(
                driver->m_em, "sock: %s: fd=%d: set sock nodelay to %s success!",
                net_endpoint_dump(net_sock_driver_tmp_buffer(driver), base_endpoint),
                endpoint->m_fd, is_enable ? "enable" : "disable");
        }
        return 0;
    }
}
