#ifndef NET_ANDROID_WATCHER_H_INCLEDED
#define NET_ANDROID_WATCHER_H_INCLEDED
#include "cpe/pal/pal_socket.h"
#include "net_android_driver_i.h"

struct net_android_watcher {
};

int net_android_watcher_init(net_watcher_t base_watcher);
void net_android_watcher_fini(net_watcher_t base_watcher);
void net_android_watcher_update(net_watcher_t base_watcher, int fd, uint8_t expect_read, uint8_t expect_write);

#endif
