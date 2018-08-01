#include "assert.h"
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils_sock/sock_utils.h"
#include "net_dgram.h"
#include "net_address.h"
#include "net_driver.h"
#include "net_dq_dgram.h"

//static void net_dq_dgram_receive_cb(EV_P_ ev_io *w, int revents);

int net_dq_dgram_init(net_dgram_t base_dgram) {
    net_schedule_t schedule = net_dgram_schedule(base_dgram);
    error_monitor_t em = net_schedule_em(schedule);
    net_dq_dgram_t dgram = net_dgram_data(base_dgram);
    net_dq_driver_t driver = net_driver_data(net_dgram_driver(base_dgram));
    net_address_t address = net_dgram_address(base_dgram);

    if (address) {
        switch(net_address_type(address)) {
        case net_address_ipv4:
            dgram->m_fd = cpe_sock_open(AF_INET, SOCK_DGRAM, 0);
            break;
        case net_address_ipv6:
            dgram->m_fd = cpe_sock_open(AF_INET6, SOCK_DGRAM, 0);
            break;
        case net_address_domain:
            CPE_ERROR(em, "dq: dgyam: not support domain address!");
            return -1;
        }
    }
    else {
        dgram->m_fd = cpe_sock_open(AF_INET, SOCK_DGRAM, 0);
    }

    if (dgram->m_fd == -1) {
        CPE_ERROR(
            em, "dq: dgram: socket create error, errno=%d (%s)",
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        return -1;
    }

    /* (driver->m_sock_process_fun) {
        if (driver->m_sock_process_fun(driver, driver->m_sock_process_ctx, dgram->m_fd, NULL) != 0) {
            CPE_ERROR(em, "dq: dgram: sock process fail");
            cpe_sock_close(dgram->m_fd);
            return -1;
        }
    }*/
    
    if (address) {
        struct sockaddr_storage addr;
        socklen_t addr_len = sizeof(addr);

        if (net_address_to_sockaddr(address, (struct sockaddr *)&addr, &addr_len) != 0) {
            CPE_ERROR(em, "dq: dgram: get sockaddr from address fail");
            cpe_sock_close(dgram->m_fd);
            return -1;
        }

        sock_set_reuseport(dgram->m_fd);

        if (cpe_bind(dgram->m_fd, (struct sockaddr *)&addr, addr_len) != 0) {
            CPE_ERROR(
                em, "dq: dgram: bind addr fail, errno=%d (%s)",
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
            cpe_sock_close(dgram->m_fd);
            dgram->m_fd = -1;
            return -1;
        }

        if (driver->m_debug) {
            CPE_INFO(
                em, "dq: dgram: bind to %s",
                net_address_dump(net_schedule_tmp_buffer(schedule), address));
        }
    }
    else {
        struct sockaddr_storage addr;
        socklen_t addr_len = sizeof(struct sockaddr_storage);
        memset(&addr, 0, addr_len);
        if (getsockname(dgram->m_fd, (struct sockaddr *)&addr, &addr_len) != 0) {
            CPE_ERROR(
                em, "dq: dgram: sockaddr error, errno=%d (%s)",
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
            cpe_sock_close(dgram->m_fd);
            dgram->m_fd = -1;
            return -1;
        }

        address = net_address_create_from_sockaddr(schedule, (struct sockaddr *)&addr, addr_len);
        if (address == NULL) {
            CPE_ERROR(em, "dq: dgram: create address fail");
            cpe_sock_close(dgram->m_fd);
            dgram->m_fd = -1;
            return -1;
        }

        if (driver->m_debug) {
            CPE_INFO(
                em, "dq: dgram: auto bind at %s",
                net_address_dump(net_schedule_tmp_buffer(schedule), address));
        }

        net_dgram_set_address(base_dgram, address);
    }
    
    /*bzero(&dgram->m_watcher, sizeof(dgram->m_watcher));
    dgram->m_watcher.data = dgram;
    ev_io_init(&dgram->m_watcher, net_dq_dgram_receive_cb, dgram->m_fd, EV_READ);
    ev_io_start(driver->m_dq_loop, &dgram->m_watcher);
*/
    return 0;
}

void net_dq_dgram_fini(net_dgram_t base_dgram) {
    net_dq_dgram_t dgram = net_dgram_data(base_dgram);
    net_dq_driver_t driver = net_driver_data(net_dgram_driver(base_dgram));

    if (dgram->m_fd != -1) {
        //ev_io_stop(driver->m_dq_loop, &dgram->m_watcher);
        cpe_sock_close(dgram->m_fd);
        dgram->m_fd = -1;
    }
}

int net_dq_dgram_send(net_dgram_t base_dgram, net_address_t target, void const * data, size_t data_len) {
    net_dq_dgram_t dgram = net_dgram_data(base_dgram);
    net_schedule_t schedule = net_dgram_schedule(base_dgram);
    net_dq_driver_t driver = net_driver_data(net_dgram_driver(base_dgram));

    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    if (net_address_to_sockaddr(target, (struct sockaddr *)&addr, &addr_len) != 0) {
        net_schedule_t schedule = net_dgram_schedule(base_dgram);
        CPE_ERROR(net_schedule_em(schedule), "dq: dgram: get address to send fail");
        return -1;
    }
    
    ssize_t nret = sendto(dgram->m_fd, data, data_len, 0, (struct sockaddr *)&addr, addr_len);
    if (nret < 0) {
        CPE_ERROR(
            net_schedule_em(schedule), "dq: dgram: send %d data to %s fail, errno=%d (%s)",
            (int)data_len, net_address_dump(net_schedule_tmp_buffer(schedule), target),
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        return -1;
    }
    else {
        if (driver->m_debug) {
            CPE_INFO(
                net_schedule_em(schedule), "dq: dgram: send %d data to %s",
                (int)data_len,
                net_address_dump(net_schedule_tmp_buffer(schedule), target));
        }

        if (driver->m_data_monitor_fun) {
            driver->m_data_monitor_fun(driver->m_data_monitor_ctx, NULL, net_data_out, (uint32_t)nret);
        }
    }

    return (int)nret;
}

/*static void net_dq_dgram_receive_cb(EV_P_ ev_io *w, int revents) {
    net_dq_dgram_t dgram = w->data;
    net_dgram_t base_dgram = net_dgram_from_data(dgram);
    net_schedule_t schedule = net_dgram_schedule(base_dgram);
    net_dq_driver_t driver = net_driver_data(net_dgram_driver(base_dgram));

    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    char buf[1500] = {0};

    int nrecv = recvfrom(dgram->m_fd, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&addr, &addr_len);
    if (nrecv < 0) {
        CPE_ERROR(
            net_schedule_em(schedule),
            "dq: dgram: receive error, errno=%d (%s)",
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        return;
    }

    net_address_t from = net_address_create_from_sockaddr(schedule, (struct sockaddr *) &addr, addr_len);

    net_dgram_recv(base_dgram, from, buf, (size_t)nrecv);

    if (driver->m_data_monitor_fun) {
        driver->m_data_monitor_fun(driver->m_data_monitor_ctx, NULL, net_data_in, (uint32_t)nrecv);
    }

    net_address_free(from);
}*/
