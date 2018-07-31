#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "net_endpoint.h"
#include "net_acceptor.h"
#include "net_driver.h"
#include "net_address.h"
#include "net_dq_acceptor_i.h"
#include "net_dq_endpoint.h"

//static void net_dq_acceptor_cb(EV_P_ ev_io *w, int revents);

int net_dq_acceptor_init(net_acceptor_t base_acceptor) {
    net_dq_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));
    net_dq_acceptor_t acceptor = net_acceptor_data(base_acceptor);
    net_address_t address = net_acceptor_address(base_acceptor);
    net_schedule_t schedule = net_acceptor_schedule(base_acceptor);
    error_monitor_t em = net_schedule_em(schedule);

    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    if (net_address_to_sockaddr(address, (struct sockaddr *)&addr, &addr_len) != 0) {
        CPE_ERROR(em, "ev: acceptor: add address fail!");
        return -1;
    }
    
    acceptor->m_fd = cpe_sock_open(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (acceptor->m_fd == -1) {
        CPE_ERROR(
            em, "ev: acceptor: socket call fail, errno=%d (%s)!",
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        return -1;
    }

    if (cpe_sock_set_reuseaddr(acceptor->m_fd, 1) != 0) {
        CPE_ERROR(
            em, "ev: acceptor: set sock reuseaddr fail, errno=%d (%s)!",
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        cpe_sock_close(acceptor->m_fd);
        return -1;
    }

    if(cpe_bind(acceptor->m_fd, (struct sockaddr *)&addr, addr_len) != 0) {
        CPE_ERROR(
            em, "ev: acceptor: bind error, errno=%d (%s)",
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        cpe_sock_close(acceptor->m_fd);
        return -1;
    }

    uint32_t accept_queue_size = 0;
    if (accept_queue_size == 0) accept_queue_size = 512;
    
    if (cpe_listen(acceptor->m_fd, accept_queue_size) != 0) {
        CPE_ERROR(
            em, "ev: acceptor: listen error, errno=%d (%s)",
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        cpe_sock_close(acceptor->m_fd);
        return -1;
    }

    // acceptor->m_watcher.data = acceptor;
    // ev_io_init(&acceptor->m_watcher, net_dq_acceptor_cb, acceptor->m_fd, EV_READ);
    // ev_io_start(driver->m_dq_loop, &acceptor->m_watcher);
    
    return 0;
}

void net_dq_acceptor_fini(net_acceptor_t base_acceptor) {
    net_dq_acceptor_t acceptor = net_acceptor_data(base_acceptor);
    net_dq_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));
    net_schedule_t schedule = net_acceptor_schedule(base_acceptor);
    mem_allocrator_t alloc = net_schedule_allocrator(schedule);

    //ev_io_stop(driver->m_dq_loop, &acceptor->m_watcher);
    cpe_sock_close(acceptor->m_fd);
}

// static void net_dq_acceptor_cb(EV_P_ ev_io *w, int revents) {
//     net_dq_acceptor_t acceptor = w->data;
//     net_acceptor_t base_acceptor = net_acceptor_from_data(acceptor);
//     net_dq_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));
//     net_schedule_t schedule = net_acceptor_schedule(base_acceptor);
//     error_monitor_t em = net_schedule_em(schedule);
//     int new_fd;

//     new_fd = cpe_accept(acceptor->m_fd, 0, 0);
//     if (new_fd == -1) {
//         CPE_ERROR(
//             em, "ev: acceptor: accept error, errno=%d (%s)",
//             cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
//         return;
//     }

//     if (cpe_sock_set_none_block(new_fd, 1) != 0) {
//         CPE_ERROR(
//             em, "ev: acceptor: set non-block fail, errno=%d (%s)",
//             cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
//         cpe_sock_close(new_fd);
//         return;
//     }

//     net_endpoint_t base_endpoint =
//         net_endpoint_create(net_driver_from_data(driver), net_endpoint_inbound, net_acceptor_protocol(base_acceptor));
//     if (base_endpoint == NULL) {
//         CPE_ERROR(em, "ev: acceptor: create endpoint fail");
//         cpe_sock_close(new_fd);
//         return;
//     }

//     net_dq_endpoint_t endpoint = net_endpoint_data(base_endpoint);
//     endpoint->m_fd = new_fd;
//     net_dq_endpoint_start_rw_watcher(driver, base_endpoint, endpoint);

//     if (net_endpoint_set_state(base_endpoint, net_endpoint_state_established) != 0) {
//         net_endpoint_free(base_endpoint);
//         return;
//     }

//     if (net_dq_endpoint_update_local_address(endpoint) != 0
//         || net_dq_endpoint_update_remote_address(endpoint) != 0)
//     {
//         net_endpoint_free(base_endpoint);
//         return;
//     }

//     if (driver->m_sock_process_fun) {
//         if (driver->m_sock_process_fun(driver, driver->m_sock_process_ctx, new_fd, net_endpoint_remote_address(base_endpoint)) != 0) {
//             CPE_ERROR(em, "ev: accept: sock process fail");
//             net_endpoint_free(base_endpoint);
//             return;
//         }
//     }
// }
