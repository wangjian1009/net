#ifndef NET_SOCK_PROGRESS_H_INCLEDED
#define NET_SOCK_PROGRESS_H_INCLEDED
#include "net_sock_driver_i.h"

struct net_sock_progress {
    FILE * m_fp;
    net_watcher_t m_watcher;
};

int net_sock_progress_init(net_progress_t base_progress, const char * cmd, const char * mode);
void net_sock_progress_fini(net_progress_t base_progress);

#endif
