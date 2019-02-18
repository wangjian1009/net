#include "assert.h"
#include "cpe/pal/pal_socket.h"
#include "net_watcher.h"
#include "net_driver.h"
#include "net_dq_watcher.h"

static void net_dq_watcher_start_w(net_dq_driver_t driver, net_dq_watcher_t watcher, net_watcher_t base_watcher);
static void net_dq_watcher_stop_w(net_dq_driver_t driver, net_dq_watcher_t watcher, net_watcher_t base_watcher);
static void net_dq_watcher_start_r(net_dq_driver_t driver, net_dq_watcher_t watcher, net_watcher_t base_watcher);
static void net_dq_watcher_stop_r(net_dq_driver_t driver, net_dq_watcher_t watcher, net_watcher_t base_watcher);

int net_dq_watcher_init(net_watcher_t base_watcher) {
    net_dq_watcher_t watcher = net_watcher_data(base_watcher);

    watcher->m_source_r = nil;
    watcher->m_source_w = nil;

    return 0;
}

void net_dq_watcher_fini(net_watcher_t base_watcher) {
    net_dq_watcher_t watcher = net_watcher_data(base_watcher);
    net_dq_driver_t driver = net_driver_data(net_watcher_driver(base_watcher));

    if (watcher->m_source_r) {
        net_dq_watcher_stop_r(driver, watcher, base_watcher);
    }

    if (watcher->m_source_w) {
        net_dq_watcher_stop_w(driver, watcher, base_watcher);
    }
}

void net_ev_watcher_update(net_watcher_t base_watcher, int fd, uint8_t expect_read, uint8_t expect_write) {
}
