#ifndef NET_SOCK_DGRAM_H_INCLEDED
#define NET_SOCK_DGRAM_H_INCLEDED
#include "cpe/pal/pal_socket.h"
#include "net_sock_driver_i.h"

struct net_sock_dgram {
    int m_fd;
    int m_domain;
    net_watcher_t m_watcher;
};

int net_sock_dgram_init(net_dgram_t base_dgram);
void net_sock_dgram_fini(net_dgram_t base_dgram);
int net_sock_dgram_send(net_dgram_t base_dgram, net_address_t target, void const * data, size_t data_len);

#endif
