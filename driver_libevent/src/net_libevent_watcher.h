#ifndef NET_LIBEVENT_WATCHER_H_INCLEDED
#define NET_LIBEVENT_WATCHER_H_INCLEDED
#include "event2/event_struct.h"
#include "net_libevent_driver_i.h"

struct net_libevent_watcher {
    struct event m_event;
};

int net_libevent_watcher_init(net_watcher_t base_watcher, int fd);
void net_libevent_watcher_fini(net_watcher_t base_watcher, int fd);
void net_libevent_watcher_update(net_watcher_t base_watcher, int fd, uint8_t expect_read, uint8_t expect_write);

#endif
