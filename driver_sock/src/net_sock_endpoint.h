#ifndef NET_SOCK_ENDPOINT_H_INCLEDED
#define NET_SOCK_ENDPOINT_H_INCLEDED
#include "net_sock_driver_i.h"

struct net_sock_endpoint {
    int m_fd;
    net_watcher_t m_watcher;
    net_timer_t m_do_read;
    net_timer_t m_do_write;
};

int net_sock_endpoint_init(net_endpoint_t base_endpoint);
void net_sock_endpoint_fini(net_endpoint_t base_endpoint);
int net_sock_endpoint_connect(net_endpoint_t base_endpoint);
void net_sock_endpoint_close(net_endpoint_t base_endpoint);
int net_sock_endpoint_update(net_endpoint_t base_endpoint);

int net_sock_endpoint_set_established(net_sock_driver_t driver, net_sock_endpoint_t endpoint, net_endpoint_t base_endpoint);
int net_sock_endpoint_update_local_address(net_sock_endpoint_t endpoint);
int net_sock_endpoint_update_remote_address(net_sock_endpoint_t endpoint);

#endif
