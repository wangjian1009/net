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

static int net_dq_endpoint_start_connect(
    net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint, net_address_t remote_address);

static void net_dq_endpoint_on_connect(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint);

static void net_dq_endpoint_start_w_connect(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint);
static void net_dq_endpoint_start_w(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint);
static void net_dq_endpoint_stop_w(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint);
static void net_dq_endpoint_start_r(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint);
static void net_dq_endpoint_stop_r(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint);
static void net_dq_endpoint_start_do_write(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint);
static void net_dq_endpoint_stop_do_write(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint);
static void net_dq_endpoint_start_do_read(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint);
static void net_dq_endpoint_stop_do_read(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint);

static void net_dq_endpoint_connect_log_connect_start(
    net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint, uint8_t is_first);
static void net_dq_endpoint_connect_log_connect_success(
    net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint);

static void net_dq_endpoint_close_sock(net_dq_driver_t driver, net_dq_endpoint_t endpoint);

int net_dq_endpoint_init(net_endpoint_t base_endpoint) {
    net_dq_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    endpoint->m_fd = -1;
    endpoint->m_source_r = nil;
    endpoint->m_source_w = nil;
    endpoint->m_source_do_write = nil;
    endpoint->m_source_do_read = nil;

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

    if (endpoint->m_source_do_write) {
        net_dq_endpoint_stop_do_write(driver, endpoint, base_endpoint);
    }

    if (endpoint->m_source_do_read) {
        net_dq_endpoint_stop_do_read(driver, endpoint, base_endpoint);
    }

    if (endpoint->m_fd != -1) {
        net_dq_endpoint_close_sock(driver, endpoint);
    }
}

int net_dq_endpoint_update(net_endpoint_t base_endpoint) {
    net_dq_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_dq_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    if (!net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write) /*有数据等待写入 */
        && endpoint->m_source_do_write == nil /*没有等待执行的写入操作 */
        && endpoint->m_source_w == nil) /*socket没有等待可以写入的操作（当前可以写入数据到socket) */
    {
        /*启动写入操作 */
        net_dq_endpoint_start_do_write(driver, endpoint, base_endpoint);
    }

    if (!net_endpoint_rbuf_is_full(base_endpoint) /*读取缓存不为空，可以读取数据 */
        && endpoint->m_source_do_read == nil /*没有等待执行的读取操作 */
        && endpoint->m_source_r == nil) /*socket上没有等待读取的操作（当前有数据可以读取) */
    {
        /*启动读取操作 */
        net_dq_endpoint_start_do_read(driver, endpoint, base_endpoint);
    }
    
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
    remote_addr = net_address_resolved(remote_addr);

    if (net_address_type(remote_addr) == net_address_domain) {
        CPE_ERROR(
            driver->m_em, "dq: %s: connect not support domain address!",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
        return -1;
    }
    
    int connect_rv;

    if (net_address_type(remote_addr) == net_address_local) {
        connect_rv = net_dq_endpoint_start_connect(driver, endpoint, base_endpoint, remote_addr);
    }
    else {
        assert(net_address_type(remote_addr) == net_address_ipv4 || net_address_type(remote_addr) == net_address_ipv6);

        switch(net_schedule_local_ip_stack(net_endpoint_schedule(base_endpoint))) {
        case net_local_ip_stack_none:
            CPE_ERROR(
                driver->m_em, "dq: %s: connect not in no network env!",
                net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
            return -1;
        case net_local_ip_stack_ipv4:
            if (net_address_type(remote_addr) == net_address_ipv6) {
                if (net_address_ipv6_is_ipv4_map(remote_addr)) {
                    net_address_t remote_addr_ipv4 = net_address_create_ipv4_from_ipv6_map(net_endpoint_schedule(base_endpoint), remote_addr);
                    if (remote_addr_ipv4 == NULL) {
                        CPE_ERROR(
                            driver->m_em, "dq: %s: convert ipv6 address to ipv4(map) fail",
                            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
                        return -1;
                    }

                    connect_rv = net_dq_endpoint_start_connect(driver, endpoint, base_endpoint, remote_addr_ipv4);

                    net_address_free(remote_addr_ipv4);
                }
                else {
                    CPE_ERROR(
                        driver->m_em, "dq: %s: can`t connect to ipv6 network in ipv4 network env!",
                        net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
                    return -1;
                }
            }
            else {
                connect_rv = net_dq_endpoint_start_connect(driver, endpoint, base_endpoint, remote_addr);
            }
            break;
        case net_local_ip_stack_ipv6:
            if (net_address_type(remote_addr) == net_address_ipv4) {
                net_address_t remote_addr_ipv6 = net_address_create_ipv6_from_ipv4_nat(net_endpoint_schedule(base_endpoint), remote_addr);
                if (remote_addr_ipv6 == NULL) {
                    CPE_ERROR(
                        driver->m_em, "dq: %s: convert ipv4 address to ipv6(nat) fail",
                        net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
                    return -1;
                }

                connect_rv = net_dq_endpoint_start_connect(driver, endpoint, base_endpoint, remote_addr_ipv6);

                net_address_free(remote_addr_ipv6);
            }
            else {
                connect_rv = net_dq_endpoint_start_connect(driver, endpoint, base_endpoint, remote_addr);
            }
            break;
        case net_local_ip_stack_dual:
            connect_rv = net_dq_endpoint_start_connect(driver, endpoint, base_endpoint, remote_addr);
            break;
        }
    }

    if (connect_rv != 0) {
        if (endpoint->m_fd == -1) {
            return -1;
        }
        
        if (cpe_sock_errno() == EINPROGRESS || cpe_sock_errno() == EWOULDBLOCK) {
            if (net_endpoint_driver_debug(base_endpoint)) {
                net_dq_endpoint_connect_log_connect_start(driver, endpoint, base_endpoint, 1);
            }

            if (net_endpoint_address(base_endpoint) == NULL) {
                net_dq_endpoint_update_local_address(endpoint);
            }
            
            net_dq_endpoint_start_w_connect(driver, endpoint, base_endpoint);
            
            return net_endpoint_set_state(base_endpoint, net_endpoint_state_connecting);
        }
        else {
            CPE_ERROR(
                driver->m_em, "dq: %s: connect error, errno=%d (%s)",
                net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));

            net_endpoint_set_error(
                base_endpoint, net_endpoint_error_source_network,
                net_endpoint_network_errno_connect_error, cpe_sock_errstr(cpe_sock_errno()));

            net_dq_endpoint_close_sock(driver, endpoint);

            return -1;
        }
    }
    else {
        /*连接成功 */
        if (net_endpoint_driver_debug(base_endpoint)) {
            net_dq_endpoint_connect_log_connect_success(driver, endpoint, base_endpoint);
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
        endpoint->m_source_r == NULL /*socket上没有等待可以读取的处理（当前可以读取) */
        && net_endpoint_state(base_endpoint) == net_endpoint_state_established /*当前状态正确 */
        && !net_endpoint_rbuf_is_full(base_endpoint) /*读取缓存没有满 */
        )
    {
        uint32_t capacity = 0;
        void * rbuf = net_endpoint_buf_alloc(base_endpoint, &capacity);
        if (rbuf == NULL) {
            if (net_endpoint_state(base_endpoint) == net_endpoint_state_deleting) {
                if (endpoint->m_fd != -1) {
                    net_dq_endpoint_close_sock(driver, endpoint);
                }
                return -1;
            }
            else {
                CPE_ERROR(
                    driver->m_em, "dq: %s: on read: endpoint rbuf full!",
                    net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                    cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
                if (endpoint->m_fd != -1) {
                    net_dq_endpoint_close_sock(driver, endpoint);
                }

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
                    driver->m_em, "dq: %s: recv %d bytes data!",
                    net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                    (int)bytes);
            }

            if (driver->m_data_monitor_fun) {
                driver->m_data_monitor_fun(driver->m_data_monitor_ctx, base_endpoint, net_data_in, (uint32_t)bytes);
            }
            
            if (net_endpoint_buf_supply(base_endpoint, net_ep_buf_read, (uint32_t)bytes) != 0) {
                if (endpoint->m_fd != -1) {
                    net_dq_endpoint_close_sock(driver, endpoint);
                }
                
                if (net_endpoint_is_active(base_endpoint)) {
                    if (!net_endpoint_have_error(base_endpoint)) {
                        net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_logic, NULL);
                    }
                    if (net_endpoint_set_state(base_endpoint, net_endpoint_state_logic_error) != 0) {
                        if (net_endpoint_driver_debug(base_endpoint)) {
                            CPE_INFO(
                                driver->m_em, "dq: %s: free for process fail!",
                                net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
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
                    driver->m_em, "dq: %s: remote disconnected(recv 0)!",
                    net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
            }
                
            net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_remote_closed, NULL);
            net_dq_endpoint_close_sock(driver, endpoint);
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_disable) != 0) return -1;
            return 0;
        }
        else {
            assert(bytes == -1);
            net_endpoint_buf_release(base_endpoint);

            if (cpe_sock_errno() == EWOULDBLOCK || cpe_sock_errno() == EINPROGRESS) {
                net_dq_endpoint_start_r(driver, endpoint, base_endpoint);
                break;
            }
            else if (cpe_sock_errno() == EINTR) {
                continue;
            }
            else {
                CPE_ERROR(
                    driver->m_em, "dq: %s: recv error, errno=%d (%s)!",
                    net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                    cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));

                net_endpoint_set_error(
                    base_endpoint, net_endpoint_error_source_network,
                    net_endpoint_network_errno_network_error, cpe_sock_errstr(cpe_sock_errno()));

                net_dq_endpoint_close_sock(driver, endpoint);
                if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) return -1;
                return 0;
            }
        }
        assert(0);
    }

    return 0;
}

int net_dq_endpoint_on_write(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint) {
    while(endpoint->m_source_w == NULL /*socket没有等待（当前可以写入数据) */
          && net_endpoint_state(base_endpoint) == net_endpoint_state_established /*ep状态正确 */
          && !net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write) /*还有数据等待写入 */
        )
    {
        uint32_t data_size = net_endpoint_buf_size(base_endpoint, net_ep_buf_write);
        void * data = net_endpoint_buf_peak(base_endpoint, net_ep_buf_write, &data_size);
        assert(data_size > 0);
        assert(data);

        ssize_t bytes = cpe_send(endpoint->m_fd, data, data_size, CPE_SOCKET_DEFAULT_SEND_FLAGS);
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

            net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_remote_closed, NULL);
            net_dq_endpoint_close_sock(driver, endpoint);
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

                net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_network_error, cpe_sock_errstr(err));
                net_dq_endpoint_close_sock(driver, endpoint);
                if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) return -1;
                return 0;
            }
            
            CPE_ERROR(
                driver->m_em, "dq: %s: send error, errno=%d (%s)!",
                net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                err, cpe_sock_errstr(err));

            net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_network_error, cpe_sock_errstr(err));
            net_dq_endpoint_close_sock(driver, endpoint);
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

        net_endpoint_set_error(
            base_endpoint, net_endpoint_error_source_network,
            net_endpoint_network_errno_network_error, cpe_sock_errstr(cpe_sock_errno()));

        net_dq_endpoint_close_sock(driver, endpoint);
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
            net_endpoint_free(base_endpoint);
        }

        return;
    }

    if (err != 0) {
        if (err == EINPROGRESS || err == EWOULDBLOCK) {
            if (net_endpoint_driver_debug(base_endpoint) >= 2) {
                net_dq_endpoint_connect_log_connect_start(driver, endpoint, base_endpoint, 1);
            }

            net_dq_endpoint_start_w_connect(driver, endpoint, base_endpoint);
        }
        else {
            CPE_ERROR(
                driver->m_em, "dq: %s: connect error(callback), errno=%d (%s)",
                net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                err, cpe_sock_errstr(err));

            net_endpoint_set_error(
                base_endpoint, net_endpoint_error_source_network,
                net_endpoint_network_errno_connect_error, cpe_sock_errstr(err));

            net_dq_endpoint_close_sock(driver, endpoint);
            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
                net_endpoint_free(base_endpoint);
            }
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

static void net_dq_endpoint_start_w_connect(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint) {
    assert(endpoint->m_source_w == nil);
    endpoint->m_source_w = dispatch_source_create(DISPATCH_SOURCE_TYPE_WRITE, endpoint->m_fd, 0, dispatch_get_main_queue());
    dispatch_source_set_event_handler(endpoint->m_source_w, ^{
            net_dq_endpoint_stop_w(driver, endpoint, base_endpoint);
            net_dq_endpoint_on_connect(driver, endpoint, base_endpoint);
        });
    dispatch_resume(endpoint->m_source_w);
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

static void net_dq_endpoint_start_do_write(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint) {
    assert(endpoint->m_source_do_write == NULL);
    
    endpoint->m_source_do_write = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());
		
    dispatch_source_set_event_handler(
        endpoint->m_source_do_write,
        ^{
            net_dq_endpoint_stop_do_write(driver, endpoint, base_endpoint);
            if (net_dq_endpoint_on_write(driver, endpoint, base_endpoint) != 0) {
                net_endpoint_free(base_endpoint);
            }
        });

    dispatch_source_set_timer(endpoint->m_source_do_write, DISPATCH_TIME_NOW, 0, 0);
    dispatch_resume(endpoint->m_source_do_write);
}

static void net_dq_endpoint_stop_do_write(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint) {
    assert(endpoint->m_source_do_write != NULL);
    
    dispatch_source_set_event_handler(endpoint->m_source_do_write, nil);
    dispatch_source_cancel(endpoint->m_source_do_write);
    dispatch_release(endpoint->m_source_do_write);
    endpoint->m_source_do_write = nil;
}

static void net_dq_endpoint_start_do_read(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint) {
    assert(endpoint->m_source_do_read == NULL);
    
    endpoint->m_source_do_read = dispatch_source_create(DISPATCH_SOURCE_TYPE_TIMER, 0, 0, dispatch_get_main_queue());

    dispatch_source_set_event_handler(
        endpoint->m_source_do_read,
        ^{
            net_dq_endpoint_stop_do_read(driver, endpoint, base_endpoint);
            if (net_dq_endpoint_on_read(driver, endpoint, base_endpoint) != 0) {
                net_endpoint_free(base_endpoint);
            }
        });

    dispatch_source_set_timer(endpoint->m_source_do_read, DISPATCH_TIME_NOW, 0, 0);
    dispatch_resume(endpoint->m_source_do_read);
}

static void net_dq_endpoint_stop_do_read(net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint) {
    assert(endpoint->m_source_do_read != NULL);
    
    dispatch_source_set_event_handler(endpoint->m_source_do_read, nil);
    dispatch_source_cancel(endpoint->m_source_do_read);
    dispatch_release(endpoint->m_source_do_read);
    endpoint->m_source_do_read = nil;
}

static void net_dq_endpoint_connect_log_connect_start(
    net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint, uint8_t is_first)
{
    net_address_t local_address = net_endpoint_address(base_endpoint);
    if (local_address) {
        char local_addr_buf[128];
        cpe_str_dup(
            local_addr_buf, sizeof(local_addr_buf),
            net_address_dump(net_dq_driver_tmp_buffer(driver), local_address));

        CPE_INFO(
            driver->m_em, "dq: %s: connect %s (local-address=%s)",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
            local_addr_buf,
            is_first ? "start" : "restart");
    }
    else {
        CPE_INFO(
            driver->m_em, "dq: %s: connect %s",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
            is_first ? "start" : "restart");
    }
}

static void net_dq_endpoint_connect_log_connect_success(
    net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint)
{
    net_address_t local_address = net_endpoint_address(base_endpoint);
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

static int net_dq_endpoint_start_connect(
    net_dq_driver_t driver, net_dq_endpoint_t endpoint, net_endpoint_t base_endpoint, net_address_t remote_addr)
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
        domain = AF_LOCAL;
        protocol = 0;
        break;
    case net_address_domain:
        CPE_ERROR(
            driver->m_em, "dq: %s: connect not support %s",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
            net_address_type_str(net_address_type(remote_addr)));
        return -1;
    }

    endpoint->m_fd = cpe_sock_open(domain, SOCK_STREAM, protocol);
    if (endpoint->m_fd == -1) {
        CPE_ERROR(
            driver->m_em, "dq: %s: create socket fail, errno=%d (%s)",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
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
                driver->m_em, "dq: %s: connect not support connect to domain address!",
                net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint));
            net_dq_endpoint_close_sock(driver, endpoint);
            return -1;
        }

        if (cpe_sock_set_reuseaddr(endpoint->m_fd, 1) != 0) {
            CPE_ERROR(
                driver->m_em, "dq: %s: set sock reuse address fail, errno=%d (%s)",
                net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
            net_dq_endpoint_close_sock(driver, endpoint);
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
            net_dq_endpoint_close_sock(driver, endpoint);
            return -1;
        }
    }

    if (cpe_sock_set_none_block(endpoint->m_fd, 1) != 0) {
        CPE_ERROR(
            driver->m_em, "dq: %s: set non-block fail, errno=%d (%s)",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        net_dq_endpoint_close_sock(driver, endpoint);
        return -1;
    }

    if (cpe_sock_set_no_sigpipe(endpoint->m_fd, 1) != 0) {
        CPE_ERROR(
            driver->m_em, "dq: %s: set no-sig-pipe fail, errno=%d (%s)",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint),
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        net_dq_endpoint_close_sock(driver, endpoint);
        return -1;
    }

    if (cpe_sock_set_send_timeout(endpoint->m_fd, 30 * 1000) != 0) {
        CPE_ERROR(
            driver->m_em, "dq: %s: fd=%d: set send timeout fail, errno=%d (%s)",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        net_dq_endpoint_close_sock(driver, endpoint);
        return -1;
    }

    if (cpe_sock_set_recv_timeout(endpoint->m_fd, 30 * 1000) != 0) {
        CPE_ERROR(
            driver->m_em, "dq: %s: fd=%d: set recv timeout fail, errno=%d (%s)",
            net_endpoint_dump(net_dq_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd,
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        net_dq_endpoint_close_sock(driver, endpoint);
        return -1;
    }
    
    return cpe_connect(endpoint->m_fd, (struct sockaddr *)&remote_addr_sock, remote_addr_sock_len);
}

static void net_dq_endpoint_close_sock(net_dq_driver_t driver, net_dq_endpoint_t endpoint) {
    assert(endpoint->m_fd != -1);
    cpe_sock_close(endpoint->m_fd);
    endpoint->m_fd = -1;
}

