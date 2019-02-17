#include "assert.h"
#include "cpe/pal/pal_strings.h"
#include "cpe/utils/string_utils.h"
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
    watcher->m_in_processing = 0;
    watcher->m_deleting = 0;

    if (driver->m_watcher_init(watcher) != 0) {
        TAILQ_INSERT_TAIL(&driver->m_free_watchers, watcher, m_next_for_driver);
        return NULL;
    }

    cpe_hash_entry_init(&watcher->m_hh);
    if (cpe_hash_table_insert_unique(&schedule->m_watchers, watcher) != 0) {
        CPE_ERROR(schedule->m_em, "watcher: id duplicate!");
        driver->m_watcher_fini(watcher);
        TAILQ_INSERT_TAIL(&driver->m_free_watchers, watcher, m_next_for_driver);
        return NULL;
    }

    TAILQ_INSERT_TAIL(&driver->m_watchers, watcher, m_next_for_driver);

    return watcher;
}

void net_watcher_free(net_watcher_t watcher) {
    net_schedule_t schedule = watcher->m_driver->m_schedule;

    if (watcher->m_in_processing) {
        watcher->m_deleting = 1;
        return;
    }
    
    watcher->m_driver->m_watcher_fini(watcher);

    cpe_hash_table_remove_by_ins(&watcher->m_driver->m_schedule->m_watchers, watcher);

    TAILQ_REMOVE(&watcher->m_driver->m_watchers, watcher, m_next_for_driver);
    TAILQ_INSERT_TAIL(&watcher->m_driver->m_free_watchers, watcher, m_next_for_driver);
}

void net_watcher_real_free(net_watcher_t watcher) {
    net_driver_t driver = watcher->m_driver;
    
    TAILQ_REMOVE(&driver->m_free_watchers, watcher, m_next_for_driver);
    mem_free(driver->m_schedule->m_alloc, watcher);
}

net_driver_t net_watcher_driver(net_watcher_t watcher) {
    return watcher->m_driver;
}

net_watcher_t net_watcher_find(net_schedule_t schedule, int fd) {
    struct net_watcher key;
    key.m_fd = fd;
    return cpe_hash_table_find(&schedule->m_watchers, &key);
}

uint8_t net_watcher_expect_read(net_watcher_t watcher) {
    return watcher->m_expect_read;
}

uint8_t net_watcher_expect_write(net_watcher_t watcher) {
    return watcher->m_expect_write;
}

void net_watcher_update(net_watcher_t watcher, uint8_t expect_read, uint8_t expect_write) {
    if (watcher->m_expect_read == expect_read && watcher->m_expect_write == expect_write) return;

    watcher->m_expect_read = expect_read;
    watcher->m_expect_write = expect_write;

    assert(watcher->m_in_processing == 0);
    watcher->m_in_processing = 1;
    watcher->m_action(watcher->m_ctx, watcher->m_fd, expect_read, expect_write);
    watcher->m_in_processing = 1;

    if (watcher->m_deleting) {
        net_watcher_free(watcher);
    }
}

uint32_t net_watcher_hash(net_watcher_t o, void * user_data) {
    return (uint32_t)o->m_fd;
}

int net_watcher_eq(net_watcher_t l, net_watcher_t r, void * user_data) {
    return l->m_fd == r->m_fd ? 1 : 0;
}
