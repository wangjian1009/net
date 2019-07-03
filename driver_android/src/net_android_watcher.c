#include "assert.h"
#include "cpe/pal/pal_strings.h"
#include "net_watcher.h"
#include "net_driver.h"
#include "net_android_watcher.h"

static int net_android_watcher_rw_cb(int fd, int events, void* data);

int net_android_watcher_init(net_watcher_t base_watcher, int fd) {
    return 0;
}

void net_android_watcher_fini(net_watcher_t base_watcher, int fd) {
    net_android_driver_t driver = net_android_driver_from_base_driver(net_watcher_driver(base_watcher));
    ALooper * looper = ALooper_forThread();

    assert(looper == driver->m_looper);

    if (ALooper_removeFd(looper, fd) < 0) {
        CPE_ERROR(driver->m_em, "android: driver: watcher fini: remove fd %d fail!", fd);
        assert(0);
    }
}

void net_android_watcher_update(net_watcher_t base_watcher, int fd, uint8_t expect_read, uint8_t expect_write) {
    net_android_driver_t driver = net_android_driver_from_base_driver(net_watcher_driver(base_watcher));
    ALooper * looper = ALooper_forThread();

    
    assert(looper = net_android_driver_from_base_driver(net_watcher_driver(base_watcher))->m_looper);

    int events = (expect_read ? ALOOPER_EVENT_INPUT : 0) | (expect_write ? ALOOPER_EVENT_OUTPUT : 0);
    
    if (!events) {
        if (ALooper_removeFd(looper, fd) < 0) {
            CPE_ERROR(driver->m_em, "android: driver: update: remove fd %d fail!", fd);
            assert(0);
        }
    }
    else {
        if (ALooper_addFd(looper, fd, 0, events, net_android_watcher_rw_cb, base_watcher) < 0) {
            CPE_ERROR(driver->m_em, "android: driver: update: add fd %d fail!", fd);
            assert(0);
        }
    }
}

static int net_android_watcher_rw_cb(int fd, int events, void* data) {
    net_watcher_t base_watcher = data;
    net_watcher_notify(base_watcher, (events | ALOOPER_EVENT_INPUT) ? 1 : 0, (events | ALOOPER_EVENT_OUTPUT) ? 1 : 0);
    return 0;
}
