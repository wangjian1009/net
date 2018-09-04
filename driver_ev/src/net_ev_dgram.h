#ifndef NET_EV_DGRAM_H_INCLEDED
#define NET_EV_DGRAM_H_INCLEDED
#include "cpe/pal/pal_socket.h"
#include "net_ev_driver_i.h"

struct net_ev_dgram {
    int m_fd;
    int m_domain;
    struct ev_io m_watcher;
};

int net_ev_dgram_init(net_dgram_t base_dgram);
void net_ev_dgram_fini(net_dgram_t base_dgram);
int net_ev_dgram_send(net_dgram_t base_dgram, net_address_t target, void const * data, size_t data_len);

#endif
