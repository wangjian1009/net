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

int net_libevent_endpoint_init(net_endpoint_t base_endpoint) {
    net_libevent_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    return 0;
}

void net_libevent_endpoint_fini(net_endpoint_t base_endpoint) {
    net_libevent_endpoint_t endpoint = net_endpoint_data(base_endpoint);
    net_libevent_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint));
}

int net_libevent_endpoint_update(net_endpoint_t base_endpoint) {
    /* net_libevent_endpoint_t endpoint = net_endpoint_data(base_endpoint); */
    /* net_libevent_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint)); */

    /* assert(net_endpoint_state(base_endpoint) == net_endpoint_state_established); */
    /* assert(endpoint->m_watcher != NULL); */

    /* if (!net_endpoint_buf_is_empty(base_endpoint, net_ep_buf_write) /\*有数据等待写入 *\/ */
    /*     && !net_watcher_expect_write(endpoint->m_watcher)) /\*socket没有等待可以写入的操作（当前可以写入数据到socket) *\/ */
    /* { */
    /*     if (net_libevent_endpoint_on_write(driver, endpoint, base_endpoint) != 0) { */
    /*         net_endpoint_set_state(base_endpoint, net_endpoint_state_deleting); */
    /*         return -1; */
    /*     } */
    /*     if (net_endpoint_state(base_endpoint) != net_endpoint_state_established) return 0; */
    /* } */

    /* assert(net_endpoint_state(base_endpoint) == net_endpoint_state_established); */
    /* assert(endpoint->m_watcher != NULL); */
    /* if (!net_watcher_expect_read(endpoint->m_watcher) /\*socket上没有等待读取的操作（当前有数据可以读取) *\/ */
    /*     && !net_endpoint_rbuf_is_full(base_endpoint) /\*读取缓存不为空，可以读取数据 *\/ */
    /*     ) */
    /* { */
    /*     if (net_endpoint_driver_debug(base_endpoint) >= 3) { */
    /*         CPE_INFO( */
    /*             driver->m_em, "sock: %s: fd=%d: wait read begin(not full)!", */
    /*             net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd); */
    /*     } */

    /*     net_watcher_update_read(endpoint->m_watcher, 1); */
    /* } */
    
    return 0;
}

int net_libevent_endpoint_connect(net_endpoint_t base_endpoint) {
    return 0;
}

void net_libevent_endpoint_close(net_endpoint_t base_endpoint) {
    /* net_libevent_endpoint_t endpoint = net_endpoint_data(base_endpoint); */
    /* net_libevent_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint)); */

    /* if (endpoint->m_fd == -1) return; */

    /* net_libevent_endpoint_close_sock(driver, endpoint); */
}

int net_libevent_endpoint_update_local_address(net_libevent_endpoint_t endpoint) {
/*     net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint); */
/*     net_libevent_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint)); */
    
/*     struct sockaddr_storage addr; */
/*     socklen_t addr_len = sizeof(struct sockaddr_storage); */
/*     memset(&addr, 0, addr_len); */
/*     if (cpe_getsockname(endpoint->m_fd, (struct sockaddr *)&addr, &addr_len) != 0) { */
/*         CPE_ERROR( */
/*             driver->m_em, "sock: %s: fd=%d: sockaddr error, errno=%d (%s)", */
/*             net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint), */
/*             endpoint->m_fd, cpe_libevent_errno(), cpe_libevent_errstr(cpe_libevent_errno())); */
/*         return -1; */
/*     } */

/*     net_address_t address = */
/*         net_address_create_from_sockaddr(net_libevent_driver_schedule(driver), (struct sockaddr *)&addr, addr_len); */
/*     if (address == NULL) { */
/*         CPE_ERROR( */
/*             driver->m_em, "sock: %s: fd=%d: create address fail", */
/*             net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd); */
/*         return -1; */
/*     } */

/*     net_endpoint_set_address(base_endpoint, address, 1); */
    
    return 0;
}

int net_libevent_endpoint_update_remote_address(net_libevent_endpoint_t endpoint) {
/*     net_endpoint_t base_endpoint = net_endpoint_from_data(endpoint); */
/*     net_libevent_driver_t driver = net_driver_data(net_endpoint_driver(base_endpoint)); */
    
/*     struct sockaddr_storage addr; */
/*     socklen_t addr_len = sizeof(struct sockaddr_storage); */
/*     memset(&addr, 0, addr_len); */
/*     if (cpe_getpeername(endpoint->m_fd, (struct sockaddr *)&addr, &addr_len) != 0) { */
/*         CPE_ERROR( */
/*             driver->m_em, "sock: %s: fd=%d: getpeername error, errno=%d (%s)", */
/*             net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd, */
/*             cpe_libevent_errno(), cpe_libevent_errstr(cpe_libevent_errno())); */
/*         return -1; */
/*     } */

/*     net_address_t address = */
/*         net_address_create_from_sockaddr(net_libevent_driver_schedule(driver), (struct sockaddr *)&addr, addr_len); */
/*     if (address == NULL) { */
/*         CPE_ERROR( */
/*             driver->m_em, "sock: %s: fd=%d: create address fail", */
/*             net_endpoint_dump(net_libevent_driver_tmp_buffer(driver), base_endpoint), endpoint->m_fd); */
/*         return -1; */
/*     } */

/*     net_endpoint_set_remote_address(base_endpoint, address, 1); */
    
    return 0;
}
