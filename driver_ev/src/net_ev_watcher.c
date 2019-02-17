#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "net_watcher.h"
#include "net_driver.h"
#include "net_ev_watcher.h"

static void net_ev_watcher_cb(EV_P_ ev_watcher *watcher, int revents);

int net_ev_watcher_init(net_watcher_t base_watcher) {
    /* net_ev_watcher_t watcher = net_watcher_data(base_watcher); */
    /* bzero(&watcher->m_watcher, sizeof(watcher->m_watcher)); */
    /* watcher->m_watcher.data = watcher; */
    return 0;
}

void net_ev_watcher_fini(net_watcher_t base_watcher) {
    /* net_ev_watcher_t watcher = net_watcher_data(base_watcher); */
    /* net_ev_driver_t driver = net_driver_data(net_watcher_driver(base_watcher)); */
    /* ev_watcher_stop(driver->m_ev_loop, &watcher->m_watcher); */
}

static void net_ev_watcher_cb(EV_P_ ev_watcher *watcher, int revents) {
}
