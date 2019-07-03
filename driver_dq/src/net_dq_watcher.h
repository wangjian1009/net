#ifndef NET_DQ_WATCHER_H_INCLEDED
#define NET_DQ_WATCHER_H_INCLEDED
#include "cpe/pal/pal_socket.h"
#include "net_dq_driver_i.h"

struct net_dq_watcher {
    dispatch_source_t m_source_r;
    dispatch_source_t m_source_w;
};

int net_dq_watcher_init(net_watcher_t base_watcher, int fd);
void net_dq_watcher_fini(net_watcher_t base_watcher, int fd);
void net_dq_watcher_update(net_watcher_t base_watcher, int fd, uint8_t expect_read, uint8_t expect_write);

#endif
