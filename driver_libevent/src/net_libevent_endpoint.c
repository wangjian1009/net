#include "event2/bufferevent.h"
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
#include "net_libevent_endpoint.h"

uint8_t net_libevent_endpoint_on_read(net_libevent_driver_t driver, net_libevent_endpoint_t endpoint, net_endpoint_t base_endpoint);
uint8_t net_libevent_endpoint_on_write(net_libevent_driver_t driver, net_libevent_endpoint_t endpoint, net_endpoint_t base_endpoint);
void net_libevent_endpoint_read_cb(struct bufferevent * bev, void *ptr);
void net_libevent_endpoint_write_cb(struct bufferevent * bev, void* ptr);
void net_libevent_endpoint_event_cb(struct bufferevent* bev, short events, void* ptr);

int net_libevent_endpoint_init(net_endpoint_t base_endpoint) {
    net_libevent_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    endpoint->m_bufferevent = NULL;
    return 0;
}

void net_libevent_endpoint_fini(net_endpoint_t base_endpoint) {
    net_libevent_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    if (endpoint->m_bufferevent) {
        bufferevent_free(endpoint->m_bufferevent);
        endpoint->m_bufferevent = NULL;
    }
}

int net_libevent_endpoint_update(net_endpoint_t base_endpoint) {
    net_libevent_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_libevent_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    assert(net_endpoint_state(base_endpoint) == net_endpoint_state_established);
    assert(endpoint->m_bufferevent != NULL);

    if (!net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write) /*有数据等待写入 */
        && !(bufferevent_get_enabled(endpoint->m_bufferevent) & EV_WRITE)) /*socket没有等待可以写入的操作（当前可以写入数据到socket) */
    {
        if (net_libevent_endpoint_on_write(driver, endpoint, base_endpoint) != 0) {
            net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
            return -1;
        }
        if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return 0;
    }

    assert(net_endpoint_state(base_endpoint) == net_endpoint_state_established);
    assert(endpoint->m_bufferevent != NULL);
    if (!(bufferevent_get_enabled(endpoint->m_bufferevent) & EV_READ) /*socket上没有等待读取的操作（当前有数据可以读取) */
        && !net_endpoint_rbuf_is_full(base_endpoint) /*读取缓存不为空，可以读取数据 */
        )
    {
        if (net_endpoint_driver_debug(base_endpoint) >= 3) {
            CPE_INFO(
                driver->m_em, "libevent: %s: fd=%d: wait read begin(not full)!",
                net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint), bufferevent_getfd(endpoint->m_bufferevent));
        }

        bufferevent_enable(endpoint->m_bufferevent, EV_READ);
    }
    
    return 0;
}

int net_libevent_endpoint_connect(net_endpoint_t base_endpoint) {
    net_libevent_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_libevent_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    net_schedule_t schedule = net_endpoint_schedule(base_endpoint);

    if (endpoint->m_bufferevent == NULL) {
        endpoint->m_bufferevent = bufferevent_socket_new(driver->m_event_base, -1, BEV_OPT_CLOSE_ON_FREE);
        if (endpoint->m_bufferevent == NULL) {
            CPE_ERROR(
                driver->m_em, "libevent: %s: fd=%d: create bufferevent fail!",
                net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
                -1);
            return -1;
        }
    }
    
CONNECT_AGAIN:
    if (bufferevent_getfd(endpoint->m_bufferevent) != -1) {
        CPE_ERROR(
            driver->m_em, "libevent: %s: fd=%d: already connected!",
            net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
            bufferevent_getfd(endpoint->m_bufferevent));
        return -1;
    }

    net_address_t remote_addr = net_endpoint_remote_address(base_endpoint);
    if (remote_addr == NULL) {
        CPE_ERROR(
            driver->m_em, "libevent: %s: fd=%d: connect with no remote address!",
            net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
            bufferevent_getfd(endpoint->m_bufferevent));
        return -1;
    }
    remote_addr = net_address_resolved(remote_addr);

    if (net_address_type(remote_addr) == net_address_domain) {
        CPE_ERROR(
            driver->m_em, "libevent: %s: fd=%d: connect not support domain address!",
            net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
            bufferevent_getfd(endpoint->m_bufferevent));
        return -1;
    }

    net_address_t connect_addr = NULL;
    
    if (net_address_type(remote_addr) == net_address_local) {
        connect_addr = remote_addr;
    }
    else {
        assert(net_address_type(remote_addr) == net_address_ipv4 || net_address_type(remote_addr) == net_address_ipv6);

        net_local_ip_stack_t ip_stack = net_schedule_local_ip_stack(schedule);
        switch(ip_stack) {
        case net_local_ip_stack_none:
            CPE_ERROR(
                driver->m_em, "libevent: %s: fd=%d: can`t connect in %s!",
                net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
                bufferevent_getfd(endpoint->m_bufferevent),
                net_local_ip_stack_str(ip_stack));
            return -1;
        case net_local_ip_stack_ipv4:
            if (net_address_type(remote_addr) == net_address_ipv6) {
                if (net_address_ipv6_is_ipv4_map(remote_addr)) {
                    connect_addr = net_address_create_ipv4_from_ipv6_map(net_endpoint_schedule(base_endpoint), remote_addr);
                    if (connect_addr == NULL) {
                        CPE_ERROR(
                            driver->m_em, "libevent: %s: fd=%d: convert ipv6 address to ipv4(map) fail",
                            net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
                            bufferevent_getfd(endpoint->m_bufferevent));
                        return -1;
                    }
                }
                else {
                    CPE_ERROR(
                        driver->m_em, "libevent: %s: fd=%d: can`t connect to ipv6 network in ipv4 network env!",
                        net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
                        bufferevent_getfd(endpoint->m_bufferevent));
                    return -1;
                }
            }
            else {
                connect_addr = remote_addr;
            }
            break;
        case net_local_ip_stack_ipv6:
            if (net_address_type(remote_addr) == net_address_ipv4) {
                connect_addr = net_address_create_ipv6_from_ipv4_nat(net_endpoint_schedule(base_endpoint), remote_addr);
                if (connect_addr == NULL) {
                    CPE_ERROR(
                        driver->m_em, "libevent: %s: fd=%d: convert ipv4 address to ipv6(nat) fail",
                        net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
                        bufferevent_getfd(endpoint->m_bufferevent));
                    return -1;
                }
            }
            else {
                connect_addr = remote_addr;
            }
            break;
        case net_local_ip_stack_dual:
            connect_addr = remote_addr;
            break;
        }
    }

    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    assert(connect_addr);

    if (net_address_to_sockaddr(connect_addr, (struct sockaddr *)&addr, &addr_len) != 0) {
        if (connect_addr != remote_addr) {
            net_address_free(connect_addr);
        }

        CPE_ERROR(
            driver->m_em, "libevent: %s: fd=%d: to sockaddr fail",
            net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
            bufferevent_getfd(endpoint->m_bufferevent));
        goto CONNECT_NEXT;
    }

    if (connect_addr != remote_addr) {
        net_address_free(connect_addr);
    }
    connect_addr = NULL;

    if (bufferevent_socket_connect(endpoint->m_bufferevent, (struct sockaddr *)&addr, addr_len) != 0) {
        CPE_ERROR(
            driver->m_em, "libevent: %s: fd=%d: connect error(first), errno=%d (%s)",
            net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
            bufferevent_getfd(endpoint->m_bufferevent),
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        goto CONNECT_NEXT;
    }
    else {
        return 0;
    }
    
CONNECT_NEXT:
    if (net_endpoint_shift_address(base_endpoint)) {
        goto CONNECT_AGAIN;
    }
    else {
        if (net_endpoint_state(base_endpoint) != net_endpoint_state_network_error
            && net_endpoint_state(base_endpoint) != net_endpoint_state_logic_error)
        {
            net_endpoint_set_error(
                base_endpoint, net_endpoint_error_source_network,
                net_endpoint_network_errno_connect_error, "TODO");

            if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) return -1;
        }

        return -1;
    }
}

void net_libevent_endpoint_close(net_endpoint_t base_endpoint) {
    net_libevent_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_libevent_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    if (endpoint->m_bufferevent == NULL) return;

    bufferevent_free(endpoint->m_bufferevent);
    endpoint->m_bufferevent = NULL;
}

int net_libevent_endpoint_set_fd(net_libevent_endpoint_t endpoint, int fd) {
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_libevent_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    if (cpe_sock_set_none_block(fd, 1) != 0) {
        CPE_ERROR(
            driver->m_em, "libevent: endpoint: set non-block fail, errno=%d (%s)",
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        return -1;
    }
    
    int options = 0;
    endpoint->m_bufferevent = bufferevent_socket_new(driver->m_event_base, fd, options);
    if (endpoint->m_bufferevent == NULL) {
        CPE_ERROR(
            driver->m_em, "libevent: endpoint: create bufferevent");
        return -1;
    }
    
    bufferevent_setcb(
        endpoint->m_bufferevent,
        net_libevent_endpoint_read_cb,
        net_libevent_endpoint_write_cb,
        net_libevent_endpoint_event_cb,
        endpoint);

    return 0;
}

int net_libevent_endpoint_set_established(net_libevent_endpoint_t endpoint) {
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_libevent_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    assert(endpoint->m_bufferevent != NULL);

    if (net_endpoint_set_state(base_endpoint, net_endpoint_state_established) != 0) return -1;

    if (net_libevent_endpoint_on_read(driver, endpoint, base_endpoint) != 0) return -1;
    
    if (net_libevent_endpoint_on_write(driver, endpoint, base_endpoint) != 0) return -1;
    
    return 0;
}

int net_libevent_endpoint_update_local_address(net_libevent_endpoint_t endpoint) {
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_libevent_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(struct sockaddr_storage);
    memset(&addr, 0, addr_len);
    if (cpe_getsockname(bufferevent_getfd(endpoint->m_bufferevent), (struct sockaddr *)&addr, &addr_len) != 0) {
        CPE_ERROR(
            driver->m_em, "libevent: %s: fd=%d: sockaddr error, errno=%d (%s)",
            net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
            bufferevent_getfd(endpoint->m_bufferevent), cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        return -1;
    }

    net_address_t address =
        net_address_create_from_sockaddr(net_libevent_driver_schedule(driver), (struct sockaddr *)&addr, addr_len);
    if (address == NULL) {
        CPE_ERROR(
            driver->m_em, "libevent: %s: fd=%d: create address fail",
            net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint), bufferevent_getfd(endpoint->m_bufferevent));
        return -1;
    }

    net_endpoint_set_address(base_endpoint, address, 1);
    
    return 0;
}

int net_libevent_endpoint_update_remote_address(net_libevent_endpoint_t endpoint) {
    net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint);
    net_libevent_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
    
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(struct sockaddr_storage);
    memset(&addr, 0, addr_len);
    if (cpe_getpeername(bufferevent_getfd(endpoint->m_bufferevent), (struct sockaddr *)&addr, &addr_len) != 0) {
        CPE_ERROR(
            driver->m_em, "libevent: %s: fd=%d: getpeername error, errno=%d (%s)",
            net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
            bufferevent_getfd(endpoint->m_bufferevent),
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        return -1;
    }

    net_address_t address =
        net_address_create_from_sockaddr(net_libevent_driver_schedule(driver), (struct sockaddr *)&addr, addr_len);
    if (address == NULL) {
        CPE_ERROR(
            driver->m_em, "libevent: %s: fd=%d: create address fail",
            net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
            bufferevent_getfd(endpoint->m_bufferevent));
        return -1;
    }

    net_endpoint_set_remote_address(base_endpoint, address, 1);
    
    return 0;
}

uint8_t net_libevent_endpoint_on_write(net_libevent_driver_t driver, net_libevent_endpoint_t endpoint, net_endpoint_t base_endpoint) {
    while(net_endpoint_state(base_endpoint) == net_endpoint_state_established /*ep状态正确 */
          && (bufferevent_get_enabled(endpoint->m_bufferevent) & EV_WRITE)
          && !net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write) /*还有数据等待写入 */
        )
    {
        
        uint32_t data_size = net_endpoint_buf_size(base_endpoint, net_ep_buf_write);
        void * data = net_endpoint_buf_peak(base_endpoint, net_ep_buf_write, &data_size);
        assert(data_size > 0);
        assert(data);

        if (bufferevent_write(endpoint->m_bufferevent, data, data_size) != 0) {
            return 0;
        }

        if (net_endpoint_driver_debug(base_endpoint)) {
            CPE_INFO(
                driver->m_em, "libevent: %s: fd=%d: send %d bytes data!",
                net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
                bufferevent_getfd(endpoint->m_bufferevent),
                data_size);
        }

        net_endpoint_buf_consume(base_endpoint, net_ep_buf_write, data_size);
    }

    return 0;
}

uint8_t net_libevent_endpoint_on_read(net_libevent_driver_t driver, net_libevent_endpoint_t endpoint, net_endpoint_t base_endpoint) {
    while(
        net_endpoint_state(base_endpoint) == net_endpoint_state_established /*当前状态正确 */
        && !net_endpoint_rbuf_is_full(base_endpoint) /*读取缓存没有满 */
        )
    {
        uint32_t capacity = 0;
        void * rbuf = net_endpoint_buf_alloc(base_endpoint, &capacity);
        if (rbuf == NULL) {
            if (endpoint->m_bufferevent) {
                bufferevent_free(endpoint->m_bufferevent);
                endpoint->m_bufferevent = NULL;
            }

            if (net_endpoint_state(base_endpoint) == net_endpoint_state_deleting) {
                return -1;
            }
            else {
                CPE_ERROR(
                    driver->m_em, "libevent: %s: fd=%d: on read: endpoint rbuf full!",
                    net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
                    endpoint->m_bufferevent ? bufferevent_getfd(endpoint->m_bufferevent) : -1);

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

        
        size_t bytes = bufferevent_read(endpoint->m_bufferevent, rbuf, capacity);
        if (net_endpoint_driver_debug(base_endpoint)) {
            CPE_INFO(
                driver->m_em, "libevent: %s: fd=%d: recv %d bytes data!",
                net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
                bufferevent_getfd(endpoint->m_bufferevent),
                (int)bytes);
        }

        if (net_endpoint_buf_supply(base_endpoint, net_ep_buf_read, (uint32_t)bytes) != 0) {
            if (endpoint->m_bufferevent) {
                bufferevent_free(endpoint->m_bufferevent);
                endpoint->m_bufferevent = NULL;
            }

            if (net_endpoint_is_active(base_endpoint)) {
                if (!net_endpoint_have_error(base_endpoint)) {
                    net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_logic, NULL);
                }
                if (net_endpoint_set_state(base_endpoint, net_endpoint_state_logic_error) != 0) {
                    if (net_endpoint_driver_debug(base_endpoint)) {
                        CPE_INFO(
                            driver->m_em, "libevent: %s: fd=%d: free for process fail!",
                            net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
                            endpoint->m_bufferevent ? bufferevent_getfd(endpoint->m_bufferevent) : -1);
                    }
                    return -1;
                }
            }
        }
    }

    return 0;
}

void net_libevent_endpoint_read_cb(struct bufferevent *bev, void *ptr) {
    net_endpoint_t base_endpoint = ptr;
    net_libevent_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_libevent_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    assert(endpoint->m_bufferevent);
    assert(net_endpoint_state(base_endpoint) == net_endpoint_state_established);

    if (net_endpoint_driver_debug(base_endpoint) >= 3) {
        CPE_INFO(
            driver->m_em, "libevent: %s: fd=%d: net can read",
            net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
            bufferevent_getfd(endpoint->m_bufferevent));
    }

    if (net_libevent_endpoint_on_read(driver, endpoint, base_endpoint) != 0) {
        net_endpoint_free(base_endpoint);
        return;
    }

    if (endpoint->m_bufferevent && net_endpoint_rbuf_is_full(base_endpoint)) {
        if (net_endpoint_driver_debug(base_endpoint) >= 3) {
            CPE_INFO(
                driver->m_em, "libevent: %s: fd=%d: wait read stop(rbuf full) ",
                net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
                bufferevent_getfd(endpoint->m_bufferevent));
        }

        bufferevent_disable(endpoint->m_bufferevent, EV_READ);
    }
}

void net_libevent_endpoint_write_cb(struct bufferevent* bev, void* ptr) {
    net_endpoint_t base_endpoint = ptr;
    net_libevent_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_libevent_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    assert(endpoint->m_bufferevent);
    assert(net_endpoint_state(base_endpoint) == net_endpoint_state_established);

    if (net_endpoint_driver_debug(base_endpoint) >= 3) {
        CPE_INFO(
            driver->m_em, "libevent: %s: fd=%d: net can write",
            net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
            bufferevent_getfd(endpoint->m_bufferevent));
    }

    bufferevent_disable(endpoint->m_bufferevent, EV_WRITE);
    if (net_libevent_endpoint_on_write(driver, endpoint, base_endpoint) != 0) {
        net_endpoint_free(base_endpoint);
        return;
    }
}

void net_libevent_endpoint_event_cb(struct bufferevent* bev, short events, void* ptr) {
    net_endpoint_t base_endpoint = ptr;
    net_libevent_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_libevent_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));

    if (events & BEV_EVENT_ERROR) {
        const char * tag = "";
        if (events & BEV_EVENT_READING) {
            tag = " read";
        }
        else if (events & BEV_EVENT_WRITING) {
            tag = " write";
        }
        
        CPE_ERROR(
            driver->m_em, "sock: %s: fd=%d: %serror, errno=%d (%s)!",
            net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
            endpoint->m_bufferevent ? bufferevent_getfd(endpoint->m_bufferevent) : -1,
            tag,
            cpe_sock_errno(),
            cpe_sock_errstr(cpe_sock_errno()));

        net_endpoint_set_error(
            base_endpoint, net_endpoint_error_source_network,
            net_endpoint_network_errno_network_error,
            cpe_sock_errstr(cpe_sock_errno()));

        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_network_error) != 0) {
            net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
            return;
        }
    }

    if (events & BEV_EVENT_TIMEOUT) {
        CPE_INFO(
            driver->m_em, "libevent: %s: fd=%d: timeout",
            net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
            endpoint->m_bufferevent ? bufferevent_getfd(endpoint->m_bufferevent) : -1);
    }

    if (events & BEV_EVENT_EOF) {
        if (net_endpoint_driver_debug(base_endpoint)) {
            CPE_INFO(
                driver->m_em, "libevent: %s: fd=%d: remote disconnected!",
                net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
                endpoint->m_bufferevent ? bufferevent_getfd(endpoint->m_bufferevent) : -1);
        }

        net_endpoint_set_error(base_endpoint, net_endpoint_error_source_network, net_endpoint_network_errno_remote_closed, NULL);
        if (net_endpoint_set_state(base_endpoint, net_endpoint_state_disable) != 0) {
            net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
            return;
        }
    }

    if (events & BEV_EVENT_CONNECTED) {
        net_address_t local_address = net_endpoint_address(base_endpoint);
        if (local_address) {
            char local_addr_buf[128];
            cpe_str_dup(
                local_addr_buf, sizeof(local_addr_buf),
                net_address_dump(net_libevent_driver_tmp_buffer(driver), local_address));

            CPE_INFO(
                driver->m_em, "libevent: %s: fd=%d: connect success (local-address=%s)",
                net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
                endpoint->m_bufferevent ? bufferevent_getfd(endpoint->m_bufferevent) : -1,
                local_addr_buf);
        }
        else {
            CPE_INFO(
                driver->m_em, "libevent: %s: fd=%d: connect success",
                net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint),
                endpoint->m_bufferevent ? bufferevent_getfd(endpoint->m_bufferevent) : -1);
        }

        if (net_libevent_endpoint_set_established(endpoint) != 0) {
            net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting);
            return;
        }
    }
}
