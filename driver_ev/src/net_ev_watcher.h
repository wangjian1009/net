#ifndef NET_EV_WATCHER_H_INCLEDED
#define NET_EV_WATCHER_H_INCLEDED
#include "cpe/pal/pal_socket.h"
#include "net_ev_driver_i.h"

struct net_ev_watcher {
    int m_fd;
    struct ev_io m_watcher;
};

int net_ev_watcher_init(net_watcher_t base_watcher);
void net_ev_watcher_fini(net_watcher_t base_watcher);
void net_ev_watcher_update(net_watcher_t base_watcher, uint8_t expect_read, uint8_t expect_write);

#endif
