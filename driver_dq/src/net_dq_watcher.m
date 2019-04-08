#include "assert.h"
#include "cpe/pal/pal_socket.h"
#include "net_watcher.h"
#include "net_driver.h"
#include "net_dq_watcher.h"

static void net_dq_watcher_start_w(net_dq_driver_t driver, int fd, net_dq_watcher_t watcher, net_watcher_t base_watcher);
static void net_dq_watcher_stop_w(net_dq_driver_t driver, net_dq_watcher_t watcher, net_watcher_t base_watcher);
static void net_dq_watcher_start_r(net_dq_driver_t driver, int fd, net_dq_watcher_t watcher, net_watcher_t base_watcher);
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

void net_dq_watcher_update(net_watcher_t base_watcher, int fd, uint8_t expect_read, uint8_t expect_write) {
    net_dq_watcher_t watcher = net_watcher_data(base_watcher);
    net_dq_driver_t driver = net_driver_data(net_watcher_driver(base_watcher));

    if (expect_read) {
        if (watcher->m_source_r == NULL) {
            net_dq_watcher_start_r(driver, fd, watcher, base_watcher);
        }
    }
    else {
        if (watcher->m_source_r) {
            net_dq_watcher_stop_r(driver, watcher, base_watcher);
        }
    }

    if (expect_write) {
        if (watcher->m_source_w == NULL) {
            net_dq_watcher_start_w(driver, fd, watcher, base_watcher);
        }
    }
    else {
        if (watcher->m_source_w) {
            net_dq_watcher_stop_w(driver, watcher, base_watcher);
        }
    }
}

static void net_dq_watcher_start_w(net_dq_driver_t driver, int fd, net_dq_watcher_t watcher, net_watcher_t base_watcher) {
    assert(watcher->m_source_w == NULL);
    
    watcher->m_source_w = dispatch_source_create(DISPATCH_SOURCE_TYPE_WRITE, fd, 0, dispatch_get_main_queue());
    dispatch_source_set_event_handler(watcher->m_source_w, ^{
            net_watcher_notify(base_watcher, 0, 1);
        });
    dispatch_resume(watcher->m_source_w);
}
    
static void net_dq_watcher_stop_w(net_dq_driver_t driver, net_dq_watcher_t watcher, net_watcher_t base_watcher) {
    assert(watcher->m_source_w != NULL);

    dispatch_source_set_event_handler(watcher->m_source_w, nil);
    dispatch_source_cancel(watcher->m_source_w);
    dispatch_release(watcher->m_source_w);
    watcher->m_source_w = nil;
}

static void net_dq_watcher_start_r(net_dq_driver_t driver, int fd, net_dq_watcher_t watcher, net_watcher_t base_watcher) {
    assert(watcher->m_source_r == NULL);
    
    watcher->m_source_r = dispatch_source_create(DISPATCH_SOURCE_TYPE_READ, fd, 0, dispatch_get_main_queue());
    dispatch_source_set_event_handler(watcher->m_source_r, ^{
            net_watcher_notify(base_watcher, 1, 0);
        });
    dispatch_resume(watcher->m_source_r);
}

static void net_dq_watcher_stop_r(net_dq_driver_t driver, net_dq_watcher_t watcher, net_watcher_t base_watcher) {
    assert(watcher->m_source_r != NULL);
    
    dispatch_source_cancel(watcher->m_source_r);
    dispatch_release(watcher->m_source_r);
    watcher->m_source_r = nil;
}
