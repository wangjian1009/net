#include "assert.h"
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils_sock/sock_utils.h"
#include "net_dgram.h"
#include "net_address.h"
#include "net_driver.h"
#include "net_dgram.h"

int net_libevent_dgram_init(net_dgram_t base_dgram) {
    return 0;
}

void net_libevent_dgram_fini(net_dgram_t base_dgram) {
    /* net_libevent_dgram_t dgram = net_dgram_data(base_dgram); */

    /* assert(dgram->m_watcher); */
    /* assert(dgram->m_fd != -1); */
    
    /* net_watcher_free(dgram->m_watcher); */
    /* dgram->m_watcher = NULL; */

    /* cpe_libevent_close(dgram->m_fd); */
    /* dgram->m_fd = -1; */
}

int net_libevent_dgram_send(net_dgram_t base_dgram, net_address_t target, void const * data, size_t data_len) {
    /* net_libevent_dgram_t dgram = net_dgram_data(base_dgram); */
    /* net_schedule_t schedule = net_dgram_schedule(base_dgram); */
    /* net_libevent_driver_t driver = net_driver_data(net_dgram_driver(base_dgram)); */

    /* net_address_t remote_addr = net_address_resolved(target); */
    /* if (net_address_type(remote_addr) != net_address_ipv4 && net_address_type(remote_addr) != net_address_ipv6) { */
    /*     CPE_ERROR( */
    /*         driver->m_em, "sock: dgram: send not support domain address %s!", */
    /*         net_address_dump(net_libevent_driver_tmp_buffer(driver), remote_addr)); */
    /*     return -1; */
    /* } */
    
    /* struct sockaddr_storage addr; */
    /* socklen_t addr_len = sizeof(addr); */
    /* if (dgram->m_domain == AF_INET) { */
    /*     if (net_address_type(remote_addr) == net_address_ipv6) { */
    /*         if (net_address_ipv6_is_ipv4_map(remote_addr)) { */
    /*             net_address_t remote_addr_ipv4 = net_address_create_ipv4_from_ipv6_map(schedule, remote_addr); */
    /*             if (remote_addr_ipv4 == NULL) { */
    /*                 CPE_ERROR( */
    /*                     driver->m_em, "sock: dgram: convert ipv6 address %s to ipv4(map) fail", */
    /*                     net_address_dump(net_libevent_driver_tmp_buffer(driver), remote_addr)); */
    /*                 return -1; */
    /*             } */

    /*             net_address_to_sockaddr(remote_addr_ipv4, (struct sockaddr *)&addr, &addr_len); */

    /*             net_address_free(remote_addr_ipv4); */
    /*         } */
    /*         else { */
    /*             CPE_ERROR( */
    /*                 driver->m_em, "sock: dgram: can`t send to %s in ipv4 network env!", */
    /*                 net_address_dump(net_libevent_driver_tmp_buffer(driver), remote_addr)); */
    /*             return -1; */
    /*         } */
    /*     } */
    /*     else { */
    /*         net_address_to_sockaddr(target, (struct sockaddr *)&addr, &addr_len); */
    /*     } */
    /* } */
    /* else { */
    /*     if (net_address_type(remote_addr) == net_address_ipv4) { */
    /*         net_address_t remote_addr_ipv6; */

    /*         net_local_ip_stack_t ipstack = net_schedule_local_ip_stack(schedule); */
    /*         if (ipstack == net_local_ip_stack_dual) { */
    /*             remote_addr_ipv6 = net_address_create_ipv6_from_ipv4_map(schedule, remote_addr); */
    /*             if (remote_addr_ipv6 == NULL) { */
    /*                 CPE_ERROR( */
    /*                     driver->m_em, "sock: dgram: convert ipv4 address %s to ipv6(map) fail", */
    /*                     net_address_dump(net_libevent_driver_tmp_buffer(driver), remote_addr)); */
    /*                 return -1; */
    /*             } */
    /*         } */
    /*         else { */
    /*             remote_addr_ipv6 = net_address_create_ipv6_from_ipv4_nat(schedule, remote_addr); */
    /*             if (remote_addr_ipv6 == NULL) { */
    /*                 CPE_ERROR( */
    /*                     driver->m_em, "sock: dgram: convert ipv4 address %s to ipv6(nat) fail", */
    /*                     net_address_dump(net_libevent_driver_tmp_buffer(driver), remote_addr)); */
    /*                 return -1; */
    /*             } */
    /*         } */
            
    /*         net_address_to_sockaddr(remote_addr_ipv6, (struct sockaddr *)&addr, &addr_len); */

    /*         net_address_free(remote_addr_ipv6); */
    /*     } */
    /*     else { */
    /*         net_address_to_sockaddr(target, (struct sockaddr *)&addr, &addr_len); */
    /*     } */
    /* } */
    
    /* int nret = (int)cpe_sendto(dgram->m_fd, data, data_len, CPE_SOCKET_DEFAULT_SEND_FLAGS, (struct sockaddr *)&addr, addr_len); */
    /* if (nret < 0) { */
    /*     CPE_ERROR( */
    /*         net_schedule_em(schedule), "sock: dgram: send %d data to %s fail, errno=%d (%s)", */
    /*         (int)data_len, net_address_dump(net_schedule_tmp_buffer(schedule), target), */
    /*         cpe_libevent_errno(), cpe_libevent_errstr(cpe_libevent_errno())); */
    /*     return -1; */
    /* } */
    /* else { */
    /*     if (net_dgram_driver_debug(base_dgram)) { */
    /*         CPE_INFO( */
    /*             net_schedule_em(schedule), "sock: dgram: send %d data to %s", */
    /*             (int)data_len, */
    /*             net_address_dump(net_schedule_tmp_buffer(schedule), target)); */
    /*     } */
    /* } */

    /* return nret; */
    return 0;
}

static void net_libevent_dgram_receive_cb(void * ctx, int fd, uint8_t do_read, uint8_t do_write) {
    /* net_libevent_dgram_t dgram = ctx; */
    /* net_dgram_t base_dgram = net_dgram_from_data(dgram); */
    /* net_schedule_t schedule = net_dgram_schedule(base_dgram); */
    /* //net_libevent_driver_t driver = net_driver_data(net_dgram_driver(base_dgram)); */

    /* struct sockaddr_storage addr; */
    /* socklen_t addr_len = sizeof(addr); */
    /* char buf[1500] = {0}; */

    /* int nrecv = (int)cpe_recvfrom(dgram->m_fd, buf, sizeof(buf) - 1, 0, (struct sockaddr *)&addr, &addr_len); */
    /* if (nrecv < 0) { */
    /*     CPE_ERROR( */
    /*         net_schedule_em(schedule), */
    /*         "sock: dgram: receive error, errno=%d (%s)", */
    /*         cpe_libevent_errno(), cpe_libevent_errstr(cpe_libevent_errno())); */
    /*     return; */
    /* } */

    /* net_address_t from = net_address_create_from_sockaddr(schedule, (struct sockaddr *) &addr, addr_len); */

    /* net_dgram_recv(base_dgram, from, buf, (size_t)nrecv); */

    /* net_address_free(from); */
}
