#include "assert.h"
#include "cpe/pal/pal_socket.h"
#include "cpe/pal/pal_strings.h"
#include "net_watcher.h"
#include "net_driver.h"
#include "net_android_watcher.h"

int net_android_watcher_init(net_watcher_t base_watcher) {
    net_android_watcher_t watcher = net_watcher_data(base_watcher);
    bzero((void*)watcher, sizeof(*watcher));
    return 0;
}

void net_android_watcher_fini(net_watcher_t base_watcher) {
}

void net_android_watcher_update(net_watcher_t base_watcher, int fd, uint8_t expect_read, uint8_t expect_write) {
}

