#include <event2/listener.h>
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/utils/file.h"
#include "net_endpoint.h"
#include "net_acceptor.h"
#include "net_driver.h"
#include "net_address.h"
#include "net_libevent_acceptor_i.h"
#include "net_libevent_endpoint.h"

void net_libevent_accept_cb(struct evconnlistener * listener, int fd, struct sockaddr* addr, int addrlen, void* arg);

int net_libevent_acceptor_init(net_acceptor_t base_acceptor) {
    net_libevent_acceptor_t acceptor = net_acceptor_data(base_acceptor);
    net_libevent_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));

    net_address_t address = net_acceptor_address(base_acceptor);

    if (net_address_type(address) == net_address_local) {
        const char * path = (const char *)net_address_data(address);
        if (file_rm(path, NULL) != 0) {
            if (errno != ENOENT) {
                CPE_ERROR(
                    driver->m_em, "libevent: acceptor: rm '%s' fail, errno=%d (%s)!",
                    path, cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
                return -1;
            }
        }
    }
    
    struct sockaddr_storage addr;
    socklen_t addr_len = sizeof(addr);
    if (net_address_to_sockaddr(address, (struct sockaddr *)&addr, &addr_len) != 0) {
        CPE_ERROR(driver->m_em, "libevent: acceptor: add address fail!");
        return -1;
    }

    uint32_t accept_queue_size = net_acceptor_queue_size(base_acceptor);
    if (accept_queue_size == 0) accept_queue_size = 512;
    
    acceptor->m_listener = evconnlistener_new_bind(
        driver->m_event_base,
        net_libevent_accept_cb, base_acceptor,
        LEV_OPT_CLOSE_ON_FREE | LEV_OPT_REUSEABLE,
        accept_queue_size, (struct sockaddr *)&addr, addr_len);
    if (acceptor->m_listener == NULL) {
        CPE_ERROR(driver->m_em, "libevent: acceptor: create listener fail!");
        return -1;
    }

    if (net_address_port(address) == 0) {
        addr_len = sizeof(addr);
        memset(&addr, 0, addr_len);
        if (cpe_getsockname(evconnlistener_get_fd(acceptor->m_listener), (struct sockaddr *)&addr, &addr_len) != 0) {
            CPE_ERROR(
                driver->m_em, "libevent: acceptor: sockaddr error, errno=%d (%s)",
                cpe_sock_errno(), cpe_sock_errstr(cpe_sock_errno()));
            evconnlistener_free(acceptor->m_listener);
            return -1;
        }

        if (((struct sockaddr*)&addr)->sa_family == AF_INET) {
            net_address_set_port(address, ntohs(((struct sockaddr_in*)&addr)->sin_port));
        }
        else {
            net_address_set_port(address, ntohs(((struct sockaddr_in6*)&addr)->sin6_port));
        }
    }
    
    return 0;
}

void net_libevent_acceptor_fini(net_acceptor_t base_acceptor) {
    net_libevent_acceptor_t acceptor = net_acceptor_data(base_acceptor);
    if (acceptor->m_listener) {
        evconnlistener_free(acceptor->m_listener);
        acceptor->m_listener = NULL;
    }
}

void net_libevent_accept_cb(struct evconnlistener * listener, int fd, struct sockaddr* addr, int addrlen, void* arg) {
    net_acceptor_t base_acceptor = arg;
    net_driver_t base_driver = net_acceptor_driver(base_acceptor);
    net_libevent_driver_t driver = net_driver_data(net_acceptor_driver(base_acceptor));
    net_libevent_acceptor_t acceptor = net_acceptor_data(base_acceptor);

    net_endpoint_t base_endpoint =
        net_endpoint_create(net_acceptor_driver(base_acceptor), net_acceptor_protocol(base_acceptor));
    if (base_endpoint == NULL) {
        CPE_ERROR(driver->m_em, "libevent: acceptor: create endpoint fail");
        cpe_sock_close(fd);
        return;
    }

    net_libevent_endpoint_t endpoint = net_endpoint_data(base_endpoint);

    if (net_libevent_endpoint_set_fd(endpoint, fd) != 0) {
        cpe_sock_close(fd);
        return;
    }
    
    if (net_libevent_endpoint_update_local_address(endpoint) != 0
        || net_libevent_endpoint_update_remote_address(endpoint) != 0)
    {
        net_endpoint_free(base_endpoint);
        return;
    }

    if (net_acceptor_on_new_endpoint(base_acceptor, base_endpoint) != 0) {
        CPE_ERROR(driver->m_em, "libevent: accept: on new endpoint fail");
        net_endpoint_free(base_endpoint);
        return;
    }

    if (net_libevent_endpoint_set_established(endpoint) != 0) {
        net_endpoint_free(base_endpoint);
        return;
    }
}
