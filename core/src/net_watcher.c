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

/* void net_watcher_update_debug_info(net_watcher_t watcher) { */
/*     net_schedule_t schedule = watcher->m_driver->m_schedule; */
/*     net_debug_setup_t debug_setup; */

/*     TAILQ_FOREACH(debug_setup, &schedule->m_debug_setups, m_next_for_schedule) { */
/*         if (!net_debug_setup_check_watcher(debug_setup, watcher)) continue; */

/*         if (debug_setup->m_protocol_debug > watcher->m_protocol_debug) { */
/*             watcher->m_protocol_debug = debug_setup->m_protocol_debug; */
/*         } */

/*         if (debug_setup->m_driver_debug > watcher->m_driver_debug) { */
/*             watcher->m_driver_debug = debug_setup->m_driver_debug; */
/*         } */
/*     } */
/* } */
