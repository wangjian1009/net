#include <assert.h>
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "net_schedule.h"
#include "net_address.h"
#include "net_watcher.h"
#include "net_ping_processor_i.h"

static void net_ping_processor_tcp_connect_close(net_ping_processor_t processor);
static int net_ping_processor_tcp_connect_connect(net_ping_mgr_t mgr, net_ping_task_t task, net_ping_processor_t processor, net_address_t remote_addr);
static void net_ping_processor_tcp_connect_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write);

int net_ping_processor_start_tcp_connect(net_ping_processor_t processor) {
    net_ping_task_t task = processor->m_task;
    net_ping_mgr_t mgr = task->m_mgr;

    net_address_t remote_addr = task->m_target;
    remote_addr = net_address_resolved(remote_addr);

    if (net_address_type(remote_addr) == net_address_domain) {
        CPE_ERROR(
            mgr->m_em, "ping: %s: start: not support address type %s!", 
            net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task),
            net_address_type_str(net_address_type(task->m_target)));
        return -1;
    }

    int connect_rv;

    if (net_address_type(remote_addr) == net_address_local) {
        connect_rv = net_ping_processor_tcp_connect_connect(mgr, task, processor, remote_addr);
    }
    else {
        assert(net_address_type(remote_addr) == net_address_ipv4 || net_address_type(remote_addr) == net_address_ipv6);

        net_local_ip_stack_t ip_stack = net_schedule_local_ip_stack(mgr->m_schedule);
        switch(ip_stack) {
        case net_local_ip_stack_none:
            CPE_ERROR(
                mgr->m_em, "ping: %s: start: can`t connect in %s!",
                net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task), net_local_ip_stack_str(ip_stack));
            return -1;
        case net_local_ip_stack_ipv4:
            if (net_address_type(remote_addr) == net_address_ipv6) {
                if (net_address_ipv6_is_ipv4_map(remote_addr)) {
                    net_address_t remote_addr_ipv4 = net_address_create_ipv4_from_ipv6_map(mgr->m_schedule, remote_addr);
                    if (remote_addr_ipv4 == NULL) {
                        CPE_ERROR(
                            mgr->m_em, "ping: %s: start: convert ipv6 address to ipv4(map) fail",
                            net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task));
                        return -1;
                    }

                    connect_rv = net_ping_processor_tcp_connect_connect(mgr, task, processor, remote_addr_ipv4);

                    net_address_free(remote_addr_ipv4);
                }
                else {
                    CPE_ERROR(
                        mgr->m_em, "ping: %s: start: connect to ipv6 network in ipv4 network env!",
                        net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task));
                    return -1;
                }
            }
            else {
                connect_rv = net_ping_processor_tcp_connect_connect(mgr, task, processor, remote_addr);
            }
            break;
        case net_local_ip_stack_ipv6:
            if (net_address_type(remote_addr) == net_address_ipv4) {
                net_address_t remote_addr_ipv6 = net_address_create_ipv6_from_ipv4_nat(mgr->m_schedule, remote_addr);
                if (remote_addr_ipv6 == NULL) {
                    CPE_ERROR(
                        mgr->m_em, "ping: %s: start: convert ipv4 address to ipv6(nat) fail",
                        net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task));
                    return -1;
                }

                connect_rv = net_ping_processor_tcp_connect_connect(mgr, task, processor, remote_addr_ipv6);

                net_address_free(remote_addr_ipv6);
            }
            else {
                connect_rv = net_ping_processor_tcp_connect_connect(mgr, task, processor, remote_addr);
            }
            break;
        case net_local_ip_stack_dual:
            connect_rv = net_ping_processor_tcp_connect_connect(mgr, task, processor, remote_addr);
            break;
        }
    }

    if (connect_rv != 0) {
        if (processor->m_tcp_connect.m_fd == -1) {
            assert(processor->m_tcp_connect.m_watcher == NULL);
            return -1;
        }
        
        if (cpe_sock_errno() == EINPROGRESS || cpe_sock_errno() == EWOULDBLOCK) {
            assert(processor->m_tcp_connect.m_watcher == NULL);
            processor->m_tcp_connect.m_watcher = net_watcher_create(
                mgr->m_driver, processor->m_tcp_connect.m_fd, processor, net_ping_processor_tcp_connect_cb);
            if (processor->m_tcp_connect.m_watcher == NULL) {
                CPE_ERROR(
                    mgr->m_em, "ping: %s: start: create watcher fail",
                    net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task));
                net_ping_processor_tcp_connect_close(processor);
                return -1;
            }

            net_watcher_update(processor->m_tcp_connect.m_watcher, 1, 1);
            return 0;
        }
        else {
            CPE_ERROR(
                mgr->m_em, "ping: %s: start: connect error(first), errno=%d (%s)",
                net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task),
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
            net_ping_processor_tcp_connect_close(processor);
            return -1;
        }
    }
    else {
        /*连接成功 */
        CPE_INFO(mgr->m_em, "ping: %s: start: connect success", net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task));

        net_ping_processor_tcp_connect_close(processor);
        net_point_processor_set_result_one(processor, net_ping_error_none, NULL, 0, 0, 0, 0);
        return 0;
    }
}

static int net_ping_processor_tcp_connect_connect(net_ping_mgr_t mgr, net_ping_task_t task, net_ping_processor_t processor, net_address_t remote_addr) {
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
#if _MSC_VER
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
        assert(0);
        return -1;
    }

    processor->m_tcp_connect.m_fd = cpe_sock_open(domain, SOCK_STREAM, protocol);
    if (processor->m_tcp_connect.m_fd == -1) {
        CPE_ERROR(
            mgr->m_em, "ping: %s: start: create socket fail, errno=%d (%s)",
            net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task),
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        net_point_processor_set_result_one(processor, net_ping_error_internal, cpe_sock_errstr(cpe_sock_errno()), 1, 0, 0, 0);
        return -1;
    }

    if (cpe_sock_set_none_block(processor->m_tcp_connect.m_fd, 1) != 0) {
        CPE_ERROR(
            mgr->m_em, "ping: %s: start: set non-block fail, errno=%d (%s)",
            net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task),
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        net_ping_processor_tcp_connect_close(processor);
        net_point_processor_set_result_one(processor, net_ping_error_internal, cpe_sock_errstr(cpe_sock_errno()), 1, 0, 0, 0);
        return -1;
    }

    if (cpe_sock_set_no_sigpipe(processor->m_tcp_connect.m_fd, 1) != 0) {
        CPE_ERROR(
            mgr->m_em, "ping: %s: start: set no-sig-pipe fail, errno=%d (%s)",
            net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task),
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        net_ping_processor_tcp_connect_close(processor);
        net_point_processor_set_result_one(processor, net_ping_error_internal, cpe_sock_errstr(cpe_sock_errno()), 1, 0, 0, 0);
        return -1;
    }

    return cpe_connect(processor->m_tcp_connect.m_fd, (struct sockaddr *)&remote_addr_sock, remote_addr_sock_len);
}

static void net_ping_processor_tcp_connect_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write) {
    net_ping_processor_t processor = ctx;
    net_ping_task_t task = processor->m_task;
    net_ping_mgr_t mgr = task->m_mgr;
    
    assert(fd == processor->m_tcp_connect.m_fd);

    int err = 0;
    socklen_t err_len = sizeof(err);
    if (cpe_getsockopt(processor->m_tcp_connect.m_fd, SOL_SOCKET, SO_ERROR, (void*)&err, &err_len) == -1) {
        CPE_ERROR(
            mgr->m_em, "ping: %s: connect-cb: get socket error fail, errno=%d (%s)",
            net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task),
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        net_ping_processor_tcp_connect_close(processor);
        net_point_processor_set_result_one(processor, net_ping_error_internal, cpe_sock_errstr(cpe_sock_errno()), 1, 0, 0, 0);
        return;
    }

    if (err != 0) {
        if (err == EINPROGRESS || err == EWOULDBLOCK) {
            assert(processor->m_tcp_connect.m_watcher);
            assert(net_watcher_expect_read(processor->m_tcp_connect.m_watcher) && net_watcher_expect_write(processor->m_tcp_connect.m_watcher));
        }
        else {
            CPE_ERROR(
                mgr->m_em, "ping: %s: connect-cb: connect error(callback), errno=%d (%s)",
                net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task),
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
            net_ping_processor_tcp_connect_close(processor);
            net_point_processor_set_result_one(processor, net_ping_error_internal, cpe_sock_errstr(cpe_sock_errno()), 1, 0, 0, 0);
        }
        return;
    }

    CPE_INFO(mgr->m_em, "ping: %s: connect-cb: connect success", net_ping_task_dump(net_ping_mgr_tmp_buffer(mgr), task));

    net_ping_processor_tcp_connect_close(processor);
    net_point_processor_set_result_one(processor, net_ping_error_none, NULL, 0, 0, 0, 0);
}

static void net_ping_processor_tcp_connect_close(net_ping_processor_t processor) {
    if (processor->m_tcp_connect.m_watcher) {
        net_watcher_free(processor->m_tcp_connect.m_watcher);
        processor->m_tcp_connect.m_watcher = NULL;
    }

    if (processor->m_tcp_connect.m_fd != -1) {
        cpe_sock_close(processor->m_tcp_connect.m_fd);
        processor->m_tcp_connect.m_fd = -1;
    }
}
