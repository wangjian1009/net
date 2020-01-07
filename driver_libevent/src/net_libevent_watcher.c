#include "assert.h"
#include "cpe/pal/pal_strings.h"
#include "net_watcher.h"
#include "net_driver.h"
#include "net_sock_driver.h"
#include "net_libevent_watcher.h"

void net_libevent_watcher_cb(evutil_socket_t fd, short events, void* arg);

int net_libevent_watcher_init(net_watcher_t base_watcher, int fd) {
    net_libevent_watcher_t watcher = net_watcher_data(base_watcher);
    bzero(&watcher->m_event, sizeof(watcher->m_event));
    return 0;
}

void net_libevent_watcher_fini(net_watcher_t base_watcher, int fd) {
    net_libevent_watcher_t watcher = net_watcher_data(base_watcher);
    if (event_initialized(&watcher->m_event)) {
        event_del(&watcher->m_event);
    }
}

void net_libevent_watcher_update(net_watcher_t base_watcher, int fd, uint8_t expect_read, uint8_t expect_write) {
    net_libevent_watcher_t watcher = net_watcher_data(base_watcher);
    net_libevent_driver_t driver = net_sock_driver_data_from_base_driver(net_watcher_driver(base_watcher));

    short events = (expect_read ? EV_READ : 0) | (expect_write ? EV_WRITE : 0);
    short cur_events = 0;

    if (event_initialized(&watcher->m_event)) {
        short v = event_get_events(&watcher->m_event);
        cur_events = v & (EV_READ | EV_WRITE);
    }

    if (events == cur_events) return;

    if (event_initialized(&watcher->m_event)) {
        event_del(&watcher->m_event);
    }

    if (events) {
        event_assign(&watcher->m_event, driver->m_event_base, fd, events | EV_PERSIST, net_libevent_watcher_cb, base_watcher);
        event_add(&watcher->m_event, NULL);
        assert(event_initialized(&watcher->m_event));
    }
    else {
        bzero(&watcher->m_event, sizeof(watcher->m_event));
        assert(!event_initialized(&watcher->m_event));
    }        
}

void net_libevent_watcher_cb(evutil_socket_t fd, short events, void * arg) {
    net_watcher_t base_watcher = arg;
    net_watcher_notify(base_watcher, (events | EV_READ) ? 1 : 0, (events | EV_WRITE) ? 1 : 0);
}
