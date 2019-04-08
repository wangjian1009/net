#include "assert.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/string_utils.h"
#include "cpe/utils/stream_buffer.h"
#include "net_watcher_i.h"
#include "net_driver_i.h"

net_watcher_t net_watcher_create(net_driver_t driver, int fd, void * ctx, net_watcher_action_fun_t action) {
    net_schedule_t schedule = driver->m_schedule;
    net_watcher_t watcher;
    uint16_t capacity = sizeof(struct net_watcher) + driver->m_watcher_capacity;
        
    watcher = TAILQ_FIRST(&driver->m_free_watchers);
    if (watcher) {
        TAILQ_REMOVE(&driver->m_free_watchers, watcher, m_next_for_driver);
    }
    else {
        watcher = mem_alloc(schedule->m_alloc, capacity);
        if (watcher == NULL) {
            CPE_ERROR(schedule->m_em, "watcher: alloc fail!");
            return NULL;
        }
    }

    bzero(watcher, capacity);

    watcher->m_driver = driver;
    watcher->m_fd = fd;
    watcher->m_ctx = ctx;
    watcher->m_action = action;
    watcher->m_in_processing = 0;
    watcher->m_deleting = 0;

    if (driver->m_watcher_init(watcher) != 0) {
        TAILQ_INSERT_TAIL(&driver->m_free_watchers, watcher, m_next_for_driver);
        return NULL;
    }

    TAILQ_INSERT_TAIL(&driver->m_watchers, watcher, m_next_for_driver);

    return watcher;
}

void net_watcher_free(net_watcher_t watcher) {
    net_driver_t driver = watcher->m_driver;

    if (watcher->m_in_processing) {
        watcher->m_driver->m_watcher_update(watcher, watcher->m_fd, 0, 0);
        watcher->m_deleting = 1;
        return;
    }

    driver->m_watcher_fini(watcher);

    TAILQ_REMOVE(&driver->m_watchers, watcher, m_next_for_driver);
    TAILQ_INSERT_TAIL(&driver->m_free_watchers, watcher, m_next_for_driver);
}

void net_watcher_real_free(net_watcher_t watcher) {
    net_driver_t driver = watcher->m_driver;
    
    TAILQ_REMOVE(&driver->m_free_watchers, watcher, m_next_for_driver);
    mem_free(driver->m_schedule->m_alloc, watcher);
}

void * net_watcher_data(net_watcher_t watcher) {
    return watcher + 1;
}

net_watcher_t net_watcher_from_data(void * data) {
    return ((net_watcher_t)data) - 1;
}

net_driver_t net_watcher_driver(net_watcher_t watcher) {
    return watcher->m_driver;
}

void net_watcher_notify(net_watcher_t watcher, uint8_t do_read, uint8_t do_write) {
    uint8_t tag_local = 0;
    if (watcher->m_in_processing == 0) {
        watcher->m_in_processing = 1;
        tag_local = 1;
    }
    
    watcher->m_action(watcher->m_ctx, watcher->m_fd, do_read, do_write);

    if (tag_local) {
        watcher->m_in_processing = 0;

        if (watcher->m_deleting) {
            net_watcher_free(watcher);
        }
    }
}

void net_watcher_update(net_watcher_t watcher, uint8_t expect_read, uint8_t expect_write) {
    if (watcher->m_deleting) return;

    uint8_t tag_local = 0;
    if (watcher->m_in_processing == 0) {
        watcher->m_in_processing = 1;
        tag_local = 1;
    }
    
    watcher->m_driver->m_watcher_update(watcher, watcher->m_fd, expect_read, expect_write);

    if (tag_local) {
        watcher->m_in_processing = 0;

        if (watcher->m_deleting) {
            net_watcher_free(watcher);
        }
    }
}

void net_watcher_print(write_stream_t ws, net_watcher_t watcher) {
    stream_printf(ws, "[watcher %d]", watcher->m_fd);
}

const char * net_watcher_dump(mem_buffer_t buff, net_watcher_t watcher) {
    struct write_stream_buffer stream = CPE_WRITE_STREAM_BUFFER_INITIALIZER(buff);

    mem_buffer_clear_data(buff);
    net_watcher_print((write_stream_t)&stream, watcher);
    stream_putc((write_stream_t)&stream, 0);
    
    return mem_buffer_make_continuous(buff, 0);
}

uint32_t net_watcher_hash(net_watcher_t o, void * user_data) {
    return (uint32_t)o->m_fd;
}

int net_watcher_eq(net_watcher_t l, net_watcher_t r, void * user_data) {
    return l->m_fd == r->m_fd ? 1 : 0;
}
