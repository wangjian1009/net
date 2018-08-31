#include "assert.h"
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils_sock/sock_utils.h"
#include "net_dgram.h"
#include "net_address.h"
#include "net_driver.h"
#include "net_dq_dgram.h"

static void net_dq_dgram_on_read(net_dq_dgram_t dgram);

int net_dq_dgram_init(net_dgram_t base_dgram) {
    net_dq_dgram_t dgram = net_dgram_data(base_dgram);
    net_dq_driver_t driver = net_driver_data(net_dgram_driver(base_dgram));
    net_address_t address = net_dgram_address(base_dgram);

    assert(address);
    
    switch(net_address_type(address)) {
    case net_address_ipv4:
        dgram->m_fd = cpe_sock_open(AF_INET, SOCK_DGRAM, 0);
        break;
    case net_address_ipv6:
        dgram->m_fd = cpe_sock_open(AF_INET6, SOCK_DGRAM, 0);
        break;
    case net_address_domain:
        CPE_ERROR(driver->m_em, "dq: dgyam: not support domain address!");
        return -1;
    }

    if (dgram->m_fd == -1) {
        CPE_ERROR(
            driver->m_em, "dq: dgram: socket create error, errno=%d (%s)",
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        return -1;
    }
    
    if (address
        && (!net_address_is_any(address) || net_address_port(address) != 0))
    {
        struct sockaddr_storage addr;
        socklen_t addr_len = sizeof(addr);

        if (net_address_to_sockaddr(address, (struct sockaddr *)&addr, &addr_len) != 0) {
            CPE_ERROR(driver->m_em, "dq: dgram: get sockaddr from address fail");
            cpe_sock_close(dgram->m_fd);
            return -1;
        }

        sock_set_reuseport(dgram->m_fd);

        if (cpe_bind(dgram->m_fd, (struct sockaddr *)&addr, addr_len) != 0) {
            CPE_ERROR(
                driver->m_em, "dq: dgram: bind addr %s fail, errno=%d (%s)",
                net_address_dump(net_dq_driver_tmp_buffer(driver), address),
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
            cpe_sock_close(dgram->m_fd);
            dgram->m_fd = -1;
            return -1;
        }

        if (net_dgram_driver_debug(base_dgram)) {
            CPE_INFO(
                driver->m_em, "dq: dgram: bind to %s",
                net_address_dump(net_dq_driver_tmp_buffer(driver), address));
        }
    }
    else {
        struct sockaddr_storage addr;
        socklen_t addr_len = sizeof(struct sockaddr_storage);
        memset(&addr, 0, addr_len);
        if (getsockname(dgram->m_fd, (struct sockaddr *)&addr, &addr_len) != 0) {
            CPE_ERROR(
                driver->m_em, "dq: dgram: sockaddr error, errno=%d (%s)",
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
            cpe_sock_close(dgram->m_fd);
            dgram->m_fd = -1;
            return -1;
        }

        address = net_address_create_from_sockaddr(net_dgram_schedule(base_dgram), (struct sockaddr *)&addr, addr_len);
        if (address == NULL) {
            CPE_ERROR(driver->m_em, "dq: dgram: create address fail");
            cpe_sock_close(dgram->m_fd);
            dgram->m_fd = -1;
            return -1;
        }

        if (net_dgram_driver_debug(base_dgram)) {
            CPE_INFO(
                driver->m_em, "dq: dgram: auto bind at %s",
                net_address_dump(net_dq_driver_tmp_buffer(driver), address));
        }

        net_dgram_set_address(base_dgram, address);
    }

    dgram->m_source_r = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, dgram->m_fd, 0, dispatch_get_main_queue());
    dispatch_source_set_event_handler(dgram->m_source_r, ^{ net_dq_dgram_on_read(dgram); });
    dispatch_resume(dgram->m_source_r);

    return 0;
}

void net_dq_dgram_fini(net_dgram_t base_dgram) {
    net_dq_dgram_t dgram = net_dgram_data(base_dgram);

    if (dgram->m_source_r) {
        dispatch_source_set_event_handler(dgram->m_source_r, nil);
        dispatch_source_cancel(dgram->m_source_r);
        dispatch_release(dgram->m_source_r);
        dgram->m_source_r = nil;
    }
    
    if (dgram->m_fd != -1) {
        cpe_sock_close(dgram->m_fd);
        dgram->m_fd = -1;
    }
}

int net_dq_dgram_send(net_dgram_t base_dgram, net_address_t target, void const * data, size_t data_len) {
    net_dq_dgram_t dgram = net_dgram_data(base_dgram);
    net_dq_driver_t driver = net_driver_data(net_dgram_driver(base_dgram));

    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    if (net_address_to_sockaddr(target, (struct sockaddr *)&addr, &addr_len) != 0) {
        CPE_ERROR(driver->m_em, "dq: dgram: get address to send fail");
        return -1;
    }
    
    ssize_t nret = sendto(dgram->m_fd, data, data_len, 0, (struct sockaddr *)&addr, addr_len);
    if (nret < 0) {
        CPE_ERROR(
            driver->m_em, "dq: dgram: send %d data to %s fail, errno=%d (%s)",
            (int)data_len, net_address_dump(net_dq_driver_tmp_buffer(driver), target),
            cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
        return -1;
    }
    else {
        if (net_dgram_driver_debug(base_dgram)) {
            CPE_INFO(
                driver->m_em, "dq: dgram: send %d data to %s",
                (int)data_len,
                net_address_dump(net_dq_driver_tmp_buffer(driver), target));
        }

        if (driver->m_data_monitor_fun) {
            driver->m_data_monitor_fun(driver->m_data_monitor_ctx, NULL, net_data_out, (uint32_t)nret);
        }
    }

    return (int)nret;
}

static void net_dq_dgram_on_read(net_dq_dgram_t dgram) {
    net_dgram_t base_dgram = net_dgram_from_data(dgram);
    net_schedule_t schedule = net_dgram_schedule(base_dgram);
    net_dq_driver_t driver = net_driver_data(net_dgram_driver(base_dgram));

    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    char buf[1500] = {0};

    ssize_t nrecv = recvfrom(dgram->m_fd, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&addr, &addr_len);
    if (nrecv < 0) {
        CPE_ERROR(
            driver->m_em,
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
}
