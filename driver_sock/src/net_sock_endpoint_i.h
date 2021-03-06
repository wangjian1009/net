#ifndef NET_SOCK_ENDPOINT_I_H_INCLEDED
#define NET_SOCK_ENDPOINT_I_H_INCLEDED
#include "net_sock_endpoint.h"
#include "net_sock_driver_i.h"

struct net_sock_endpoint {
    int m_fd;
    uint8_t m_read_closed;
    uint8_t m_write_closed;
    net_timer_t m_connect_timeout;
    net_watcher_t m_watcher;
    net_address_t m_local_address_auto;
};

int net_sock_endpoint_init(net_endpoint_t base_endpoint);
void net_sock_endpoint_fini(net_endpoint_t base_endpoint);
void net_sock_endpoint_calc_size(net_endpoint_t base_endpoint, net_endpoint_size_info_t size_info);
int net_sock_endpoint_connect(net_endpoint_t base_endpoint);
void net_sock_endpoint_close(net_endpoint_t base_endpoint);
int net_sock_endpoint_update(net_endpoint_t base_endpoint);
int net_sock_endpoint_set_no_delay(net_endpoint_t endpoint, uint8_t is_enable);
int net_sock_endpoint_get_mss(net_endpoint_t endpoint, uint32_t *mss);

int net_sock_endpoint_set_established(net_sock_driver_t driver, net_sock_endpoint_t endpoint, net_endpoint_t base_endpoint);
int net_sock_endpoint_update_local_address(net_sock_endpoint_t endpoint);
int net_sock_endpoint_update_remote_address(net_sock_endpoint_t endpoint);

#endif
