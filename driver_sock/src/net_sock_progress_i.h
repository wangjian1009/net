#ifndef NET_SOCK_PROGRESS_H_INCLEDED
#define NET_SOCK_PROGRESS_H_INCLEDED
#include "net_sock_driver_i.h"

struct net_sock_progress {
    int m_child_pid;
    int m_fd;
    net_watcher_t m_watcher;
};

int net_sock_progress_init(
    net_progress_t base_progress,
    const char * cmd, const char * argv[],
    net_progress_runing_mode_t mode);

void net_sock_progress_fini(net_progress_t base_progress);

#endif
