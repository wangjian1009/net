#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/file.h"
#include "net_endpoint.h"
#include "net_acceptor.h"
#include "net_driver.h"
#include "net_address.h"
#include "net_watcher.h"
#include "net_sock_acceptor_i.h"
#include "net_sock_endpoint.h"

static void net_sock_acceptor_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write);

int net_sock_acceptor_init(net_acceptor_t base_acceptor) {
    net_driver_t base_driver = net_acceptor_driver(base_acceptor);
    net_sock_driver_t driver = net_driver_data(base_driver);
    net_sock_acceptor_t acceptor = net_acceptor_data(base_acceptor);
    net_address_t address = net_acceptor_address(base_acceptor);
    net_schedule_t schedule = net_acceptor_schedule(base_acceptor);
    error_monitor_t em = net_schedule_em(schedule);

    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    if (net_address_to_sockaddr(address, (struct sockaddr *)&addr, &addr_len) != 0) {
        CPE_ERROR(em, "sock: acceptor: add address fail!");
        return -1;
    }

    acceptor->m_fd =
        net_address_type(address) == net_address_local
        ? cpe_sock_open(AF_LOCAL, SOCK_STREAM, 0)
        : cpe_sock_open(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (acceptor->m_fd == -1) {
        CPE_ERROR(
            em, "sock: acceptor: socket call fail, errno=%d (%s)!",
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        return -1;
    }

    if (net_address_type(address) == net_address_local) {
        const char * path = (const char *)net_address_data(address);
        if (file_rm(path, NULL) != 0) {
            if (errno != ENOENT) {
                CPE_ERROR(
                    driver->m_em, "dq: acceptor: rm '%s' fail, errno=%d (%s)!",
                    path, cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
                cpe_sock_close(acceptor->m_fd);
                return -1;
            }
        }
    }
    else {
        if (cpe_sock_set_reuseaddr(acceptor->m_fd, 1) != 0) {
            CPE_ERROR(
                em, "sock: acceptor: set sock reuseaddr fail, errno=%d (%s)!",
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
            cpe_sock_close(acceptor->m_fd);
            return -1;
        }
    }

    if(cpe_bind(acceptor->m_fd, (struct sockaddr *)&addr, addr_len) != 0) {
        CPE_ERROR(
            em, "sock: acceptor: bind error, errno=%d (%s)",
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        cpe_sock_close(acceptor->m_fd);
        return -1;
    }

    uint32_t accept_queue_size = net_acceptor_queue_size(base_acceptor);
    if (accept_queue_size == 0) accept_queue_size = 512;

    if (driver->m_sock_process_fun) {
        if (driver->m_sock_process_fun(driver, driver->m_sock_process_ctx, acceptor->m_fd, NULL) != 0) {
            CPE_ERROR(em, "sock: acceptor: sock process fail");
            cpe_sock_close(acceptor->m_fd);
            return -1;
        }
    }
    
    if (cpe_listen(acceptor->m_fd, accept_queue_size) != 0) {
        CPE_ERROR(
            em, "sock: acceptor: listen error, errno=%d (%s)",
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        cpe_sock_close(acceptor->m_fd);
        return -1;
    }

    if (net_address_port(address) == 0) {
        addr_len = sizeof(addr);
        memset(&addr, 0, addr_len);
        if (cpe_getsockname(acceptor->m_fd, (struct sockaddr *)&addr, &addr_len) != 0) {
            CPE_ERROR(
                driver->m_em, "sock: acceptor: sockaddr error, errno=%d (%s)",
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
            cpe_sock_close(acceptor->m_fd);
            return -1;
        }

        if (((struct sockaddr*)&addr)->sa_family == AF_INET) {
            net_address_set_port(address, ntohs(((struct sockaddr_in*)&addr)->sin_port));
        }
        else {
            net_address_set_port(address, ntohs(((struct sockaddr_in6*)&addr)->sin6_port));
        }
    }

    acceptor->m_watcher = net_watcher_create(base_driver, acceptor->m_fd, acceptor, net_sock_acceptor_cb);
    if (acceptor->m_watcher == NULL) {
        CPE_ERROR(
            driver->m_em, "sock: acceptor: create watcher fail",
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        cpe_sock_close(acceptor->m_fd);
        return -1;
    }
    
    net_watcher_update(acceptor->m_watcher, 1, 0);
    
    return 0;
}

void net_sock_acceptor_fini(net_acceptor_t base_acceptor) {
    net_sock_acceptor_t acceptor = net_acceptor_data(base_acceptor);
    net_sock_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));

    net_watcher_free(acceptor->m_watcher);
    cpe_sock_close(acceptor->m_fd);
}

static void net_sock_acceptor_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write) {
    net_sock_acceptor_t acceptor = ctx;
    net_acceptor_t base_acceptor = net_acceptor_from_data(acceptor);
    net_sock_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));
    net_schedule_t schedule = net_acceptor_schedule(base_acceptor);
    error_monitor_t em = net_schedule_em(schedule);
    int new_fd;

    new_fd = cpe_accept(acceptor->m_fd, 0, 0);
    if (new_fd == -1) {
        CPE_ERROR(
            em, "sock: acceptor: accept error, errno=%d (%s)",
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        return;
    }

    if (cpe_sock_set_none_block(new_fd, 1) != 0) {
        CPE_ERROR(
            em, "sock: acceptor: set non-block fail, errno=%d (%s)",
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        cpe_sock_close(new_fd);
        return;
    }

    net_endpoint_t base_endpoint =
        net_endpoint_create(net_driver_from_data(driver), net_acceptor_protocol(base_acceptor));
    if (base_endpoint == NULL) {
        CPE_ERROR(em, "sock: acceptor: create endpoint fail");
        cpe_sock_close(new_fd);
        return;
    }

    net_sock_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    endpoint->m_fd = new_fd;
    net_sock_endpoint_update_rw_watcher(driver, base_endpoint, endpoint);

    if (net_endpoint_set_state(base_endpoint, net_endpoint_state_established) != 0) {
        net_endpoint_free(base_endpoint);
        return;
    }

    if (net_sock_endpoint_update_local_address(endpoint) != 0
        || net_sock_endpoint_update_remote_address(endpoint) != 0)
    {
        net_endpoint_free(base_endpoint);
        return;
    }

    if (driver->m_sock_process_fun) {
        if (driver->m_sock_process_fun(driver, driver->m_sock_process_ctx, new_fd, net_endpoint_remote_address(base_endpoint)) != 0) {
            CPE_ERROR(em, "sock: accept: sock process fail");
            net_endpoint_free(base_endpoint);
            return;
        }
    }

    if (net_acceptor_on_new_endpoint(base_acceptor, base_endpoint) != 0) {
        CPE_ERROR(em, "sock: accept: on new endpoint fail");
        net_endpoint_free(base_endpoint);
        return;
    }
}
