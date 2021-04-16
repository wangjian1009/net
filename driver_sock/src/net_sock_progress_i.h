#ifndef NET_SOCK_PROGRESS_H_INCLEDED
#define NET_SOCK_PROGRESS_H_INCLEDED
#include "cpe/pal/pal_socket.h"
#include "net_sock_driver_i.h"

struct net_sock_progress {
    int m_fd;
    net_watcher_t m_watcher;
};

int net_sock_progress_init(net_progress_t base_progress);
void net_sock_progress_fini(net_progress_t base_progress);

#endif
