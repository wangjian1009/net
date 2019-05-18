#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "net_watcher.h"
#include "net_sock_driver.h"
#include "net_ev_watcher.h"

static void net_ev_watcher_cb(EV_P_ ev_io *watcher, int revents);

int net_ev_watcher_init(net_watcher_t base_watcher) {
    net_ev_watcher_t watcher = net_watcher_data(base_watcher);
    bzero(&watcher->m_watcher, sizeof(watcher->m_watcher));
    watcher->m_watcher.data = watcher;
    return 0;
}

void net_ev_watcher_fini(net_watcher_t base_watcher) {
    net_ev_watcher_t watcher = net_watcher_data(base_watcher);
    net_ev_driver_t driver = net_sock_driver_data_from_base_driver(net_watcher_driver(base_watcher));
    ev_io_stop(driver->m_ev_loop, &watcher->m_watcher);
}

void net_ev_watcher_update(net_watcher_t base_watcher, int fd, uint8_t expect_read, uint8_t expect_write) {
    net_ev_watcher_t watcher = net_watcher_data(base_watcher);
    net_ev_driver_t driver = net_sock_driver_data_from_base_driver(net_watcher_driver(base_watcher));
    ev_io_stop(driver->m_ev_loop, &watcher->m_watcher);

    int kind = (expect_read ? EV_READ : 0) | (expect_write ? EV_WRITE : 0);
    if (kind) {
#ifdef _WIN32
        ev_io_init(&watcher->m_watcher, net_ev_watcher_cb, _open_osfhandle(fd, 0) , kind);
#else
        ev_io_init(&watcher->m_watcher, net_ev_watcher_cb, fd, kind);
#endif
        ev_io_start(driver->m_ev_loop, &watcher->m_watcher);
    }
}

static void net_ev_watcher_cb(EV_P_ ev_io * iow, int revents) {
    net_ev_watcher_t watcher = iow->data;
    net_watcher_t base_watcher = net_watcher_from_data(watcher);

    net_watcher_notify(base_watcher, (revents | EV_READ) ? 1 : 0, (revents | EV_WRITE) ? 1 : 0);
}
