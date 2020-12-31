#include "net_watcher.h"
#include "test_net_watcher.h"

int test_net_watcher_init(net_watcher_t base_watcher, int fd) {
    test_net_driver_t driver = net_driver_data(net_watcher_driver(base_watcher));
    test_net_watcher_t watcher = net_watcher_data(base_watcher);
    TAILQ_INSERT_TAIL(&driver->m_watchers, watcher, m_next);
    return 0;
}

void test_net_watcher_fini(net_watcher_t base_watcher, int fd) {
    test_net_driver_t driver = net_driver_data(net_watcher_driver(base_watcher));
    test_net_watcher_t watcher = net_watcher_data(base_watcher);
    TAILQ_REMOVE(&driver->m_watchers, watcher, m_next);
}

void test_net_watcher_update(net_watcher_t base_watcher, int fd, uint8_t expect_read, uint8_t expect_write) {
}

