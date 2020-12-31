#ifndef TEST_UTILS_NET_WATCHER_H_INCLEDED
#define TEST_UTILS_NET_WATCHER_H_INCLEDED
#include "net_watcher.h"
#include "test_net_driver.h"

struct test_net_watcher {
    TAILQ_ENTRY(test_net_watcher) m_next;
};

int test_net_watcher_init(net_watcher_t base_watcher, int fd);
void test_net_watcher_fini(net_watcher_t base_watcher, int fd);
void test_net_watcher_update(net_watcher_t base_watcher, int fd, uint8_t expect_read, uint8_t expect_write);

#endif
