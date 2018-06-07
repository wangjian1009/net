#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "net_endpoint.h"
#include "net_driver.h"
#include "net_address.h"
#include "net_ev_acceptor_i.h"
#include "net_ev_endpoint.h"

static void net_ev_acceptor_cb(EV_P_ ev_io *w, int revents);

net_ev_acceptor_t
net_ev_acceptor_create(
    net_ev_driver_t driver, net_protocol_t protocol,
    net_address_t address, uint32_t accept_queue_size)
{
    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));
    mem_allocrator_t alloc = net_schedule_allocrator(schedule);
    error_monitor_t em = net_schedule_em(schedule);

    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    if (net_address_to_sockaddr(address, (struct sockaddr *)&addr, &addr_len) != 0) {
        CPE_ERROR(em, "ev: acceptor: add address fail!");
        return NULL;
    }
    
    net_ev_acceptor_t acceptor = mem_alloc(alloc, sizeof(struct net_ev_acceptor));
    if (acceptor == NULL) {
        CPE_ERROR(em, "ev: acceptor: alloc fail!");
        return NULL;
    }

    acceptor->m_driver = driver;
    acceptor->m_address = address;
    acceptor->m_protocol = protocol;
    
    acceptor->m_fd = cpe_sock_open(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (acceptor->m_fd == -1) {
        CPE_ERROR(
            em, "ev: acceptor: socket call fail, errno=%d (%s)!",
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        mem_free(alloc, acceptor);
        return NULL;
    }

    if (cpe_sock_set_reuseaddr(acceptor->m_fd, 1) != 0) {
        CPE_ERROR(
            em, "ev: acceptor: set sock reuseaddr fail, errno=%d (%s)!",
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        cpe_sock_close(acceptor->m_fd);
        mem_free(alloc, acceptor);
        return NULL;
    }

    if(cpe_bind(acceptor->m_fd, (struct sockaddr *)&addr, addr_len) != 0) {
        CPE_ERROR(
            em, "ev: acceptor: bind error, errno=%d (%s)",
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        cpe_sock_close(acceptor->m_fd);
        mem_free(alloc, acceptor);
        return NULL;
    }

    if (accept_queue_size == 0) accept_queue_size = 512;

    if (driver->m_sock_process_fun) {
        if (driver->m_sock_process_fun(driver, driver->m_sock_process_ctx, acceptor->m_fd, NULL) != 0) {
            CPE_ERROR(em, "ev: acceptor: sock process fail");
            cpe_sock_close(acceptor->m_fd);
            mem_free(alloc, acceptor);
            return NULL;
        }
    }
    
    if (cpe_listen(acceptor->m_fd, accept_queue_size) != 0) {
        CPE_ERROR(
            em, "ev: acceptor: listen error, errno=%d (%s)",
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        cpe_sock_close(acceptor->m_fd);
        mem_free(alloc, acceptor);
        return NULL;
    }

    acceptor->m_watcher.data = acceptor;
    ev_io_init(&acceptor->m_watcher, net_ev_acceptor_cb, acceptor->m_fd, EV_READ);
    ev_io_start(driver->m_ev_loop, &acceptor->m_watcher);

    TAILQ_INSERT_TAIL(&driver->m_acceptors, acceptor, m_next_for_driver);

    if (driver->m_debug || net_schedule_debug(schedule)) {
        CPE_INFO(
            em, "ev: acceptor: listen at %s (accept-queue-size=%d)",
            net_address_dump(net_schedule_tmp_buffer(schedule), address), accept_queue_size);
    }
    
    return acceptor;
}

void net_ev_acceptor_free(net_ev_acceptor_t acceptor) {
    net_ev_driver_t driver = acceptor->m_driver;
    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));
    mem_allocrator_t alloc = net_schedule_allocrator(schedule);

    ev_io_stop(driver->m_ev_loop, &acceptor->m_watcher);

    cpe_sock_close(acceptor->m_fd);
    
    if (acceptor->m_address) {
        net_address_free(acceptor->m_address);
        acceptor->m_address = NULL;
    }
    
    TAILQ_REMOVE(&driver->m_acceptors, acceptor, m_next_for_driver);
    
    mem_free(alloc, acceptor);
}

static void net_ev_acceptor_cb(EV_P_ ev_io *w, int revents) {
    net_ev_acceptor_t acceptor = w->data;
    net_ev_driver_t driver = acceptor->m_driver;
    net_schedule_t schedule = net_driver_schedule(net_driver_from_data(driver));
    error_monitor_t em = net_schedule_em(schedule);
    int new_fd;

    new_fd = cpe_accept(acceptor->m_fd, 0, 0);
    if (new_fd == -1) {
        CPE_ERROR(
            em, "ev: acceptor: accept error, errno=%d (%s)",
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        return;
    }

    if (cpe_sock_set_none_block(new_fd, 1) != 0) {
        CPE_ERROR(
            em, "ev: acceptor: set non-block fail, errno=%d (%s)",
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        cpe_sock_close(new_fd);
        return;
    }

    net_endpoint_t base_endpoint =
        net_endpoint_create(net_driver_from_data(driver), net_endpoint_inbound, acceptor->m_protocol);
    if (base_endpoint == NULL) {
        CPE_ERROR(em, "ev: acceptor: create endpoint fail");
        cpe_sock_close(new_fd);
        return;
    }

    net_ev_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    endpoint->m_fd = new_fd;
    net_ev_endpoint_start_rw_watcher(driver, base_endpoint, endpoint);
    net_endpoint_set_state(base_endpoint, net_endpoint_state_established);

    if (net_ev_endpoint_update_local_address(endpoint) != 0
        || net_ev_endpoint_update_remote_address(endpoint) != 0)
    {
        net_endpoint_free(base_endpoint);
        return;
    }

    if (driver->m_sock_process_fun) {
        if (driver->m_sock_process_fun(driver, driver->m_sock_process_ctx, new_fd, net_endpoint_remote_address(base_endpoint)) != 0) {
            CPE_ERROR(em, "ev: accept: sock process fail");
            net_endpoint_free(base_endpoint);
            return;
        }
    }
}

