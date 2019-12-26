#include <assert.h>
#include "cpe/pal/pal_string.h"
#include "cpe/utils/string_utils.h"
#include "net_driver_i.h"
#include "net_acceptor_i.h"
#include "net_endpoint_i.h"
#include "net_dgram_i.h"
#include "net_watcher_i.h"
#include "net_timer_i.h"

net_driver_t
net_driver_create(
    net_schedule_t schedule,
    /*driver*/
    const char * driver_name,
    uint16_t driver_capacity,
    net_driver_init_fun_t driver_init,
    net_driver_fini_fun_t driver_fini,
    /*timer*/
    uint16_t timer_capacity,
    net_timer_init_fun_t timer_init,
    net_timer_fini_fun_t timer_fini,
    net_timer_schedule_fun_t timer_schedule,
    net_timer_cancel_fun_t timer_cancel,
    net_timer_is_active_fun_t timer_is_active,
    /*acceptor*/
    uint16_t acceptor_capacity,
    net_acceptor_init_fun_t acceptor_init,
    net_acceptor_fini_fun_t acceptor_fini,
    /*endpoint*/
    uint16_t endpoint_capacity,
    net_endpoint_init_fun_t endpoint_init,
    net_endpoint_fini_fun_t endpoint_fini,
    net_endpoint_connect_fun_t endpoint_connect,
    net_endpoint_close_fun_t endpoint_close,
    net_endpoint_update_fun_t endpoint_update,
    /*dgram*/
    uint16_t dgram_capacity,
    net_dgram_init_fun_t dgram_init,
    net_dgram_fini_fun_t dgram_fini,
    net_dgram_send_fun_t dgram_send,
    /*watcher*/
    uint16_t watcher_capacity,
    net_watcher_init_fun_t watcher_init,
    net_watcher_fini_fun_t watcher_fini,
    net_watcher_update_fun_t watcher_update)
{
    net_driver_t driver;
    
    driver = mem_alloc(schedule->m_alloc, sizeof(struct net_driver) + driver_capacity);
    if (driver == NULL) {
        CPE_ERROR(schedule->m_em, "core: driver alloc fail!");
        return NULL;
    }

    driver->m_schedule = schedule;
    cpe_str_dup(driver->m_name, sizeof(driver->m_name), driver_name);
    driver->m_debug = 0;

    /*driver*/
    driver->m_driver_capacity = driver_capacity;
    driver->m_driver_init = driver_init;
    driver->m_driver_fini = driver_fini;

    /*timer*/
    driver->m_timer_capacity = timer_capacity;
    driver->m_timer_init = timer_init;
    driver->m_timer_fini = timer_fini;
    driver->m_timer_schedule = timer_schedule;
    driver->m_timer_cancel = timer_cancel;
    driver->m_timer_is_active = timer_is_active;

    /*acceptor*/
    driver->m_acceptor_capacity = acceptor_capacity;
    driver->m_acceptor_init = acceptor_init;
    driver->m_acceptor_fini = acceptor_fini;
    
    /*endpoint*/
    driver->m_endpoint_capacity = endpoint_capacity;
    driver->m_endpoint_init = endpoint_init;
    driver->m_endpoint_fini = endpoint_fini;
    driver->m_endpoint_connect = endpoint_connect;
    driver->m_endpoint_close = endpoint_close;
    driver->m_endpoint_update = endpoint_update;

    /*dgram*/
    driver->m_dgram_capacity = dgram_capacity;
    driver->m_dgram_init = dgram_init;
    driver->m_dgram_fini = dgram_fini;
    driver->m_dgram_send = dgram_send;

    /*watcher*/
    driver->m_watcher_capacity = watcher_capacity;
    driver->m_watcher_init = watcher_init;
    driver->m_watcher_fini = watcher_fini;
    driver->m_watcher_update = watcher_update;
    
    /*runtime*/
    TAILQ_INIT(&driver->m_free_acceptors);
    TAILQ_INIT(&driver->m_acceptors);

    TAILQ_INIT(&driver->m_free_endpoints);
    TAILQ_INIT(&driver->m_endpoints);
    TAILQ_INIT(&driver->m_deleting_endpoints);
    
    TAILQ_INIT(&driver->m_free_dgrams);
    TAILQ_INIT(&driver->m_dgrams);

    TAILQ_INIT(&driver->m_free_watchers);
    TAILQ_INIT(&driver->m_watchers);
    
    TAILQ_INIT(&driver->m_free_timers);
    TAILQ_INIT(&driver->m_timers);
    
    if (driver_init(driver) != 0) {
        mem_free(schedule->m_alloc, driver);
        return NULL;
    }
    
    TAILQ_INSERT_TAIL(&schedule->m_drivers, driver, m_next_for_schedule);

    if (schedule->m_debug) {
        CPE_INFO(schedule->m_em, "core: driver %s created", driver_name);
    }
    
    return driver;
}

void net_driver_free(net_driver_t driver) {
    net_schedule_t schedule = driver->m_schedule;

    if (schedule->m_direct_driver == driver) {
        schedule->m_direct_driver = NULL;
    }
    
    while(!TAILQ_EMPTY(&driver->m_acceptors)) {
        net_acceptor_free(TAILQ_FIRST(&driver->m_acceptors));
    }

    while(!TAILQ_EMPTY(&driver->m_endpoints)) {
        net_endpoint_free(TAILQ_FIRST(&driver->m_endpoints));
    }

    while(!TAILQ_EMPTY(&driver->m_deleting_endpoints)) {
        net_endpoint_free(TAILQ_FIRST(&driver->m_deleting_endpoints));
    }

    while(!TAILQ_EMPTY(&driver->m_dgrams)) {
        net_dgram_free(TAILQ_FIRST(&driver->m_dgrams));
    }

    while(!TAILQ_EMPTY(&driver->m_watchers)) {
        net_watcher_free(TAILQ_FIRST(&driver->m_watchers));
    }
    
    while(!TAILQ_EMPTY(&driver->m_timers)) {
        net_timer_free(TAILQ_FIRST(&driver->m_timers));
    }

    if (driver->m_driver_fini) {
        driver->m_driver_fini(driver);
    }

    while(!TAILQ_EMPTY(&driver->m_free_acceptors)) {
        net_acceptor_real_free(TAILQ_FIRST(&driver->m_free_acceptors));
    }
    
    while(!TAILQ_EMPTY(&driver->m_free_endpoints)) {
        net_endpoint_real_free(TAILQ_FIRST(&driver->m_free_endpoints));
    }
    
    while(!TAILQ_EMPTY(&driver->m_free_dgrams)) {
        net_dgram_real_free(TAILQ_FIRST(&driver->m_free_dgrams));
    }

    while(!TAILQ_EMPTY(&driver->m_free_watchers)) {
        net_watcher_real_free(TAILQ_FIRST(&driver->m_free_watchers));
    }
    
    while(!TAILQ_EMPTY(&driver->m_free_timers)) {
        net_timer_real_free(TAILQ_FIRST(&driver->m_free_timers));
    }
    
    TAILQ_REMOVE(&schedule->m_drivers, driver, m_next_for_schedule);
    
    mem_free(schedule->m_alloc, driver);
}

net_driver_t
net_driver_find(net_schedule_t schedule, const char * driver_name) {
    net_driver_t driver;

    TAILQ_FOREACH(driver, &schedule->m_drivers, m_next_for_schedule) {
        if (strcmp(driver->m_name, driver_name) == 0) return driver;
    }

    return NULL;
}

net_schedule_t net_driver_schedule(net_driver_t driver) {
    return driver->m_schedule;
}

const char * net_driver_name(net_driver_t driver) {
    return driver->m_name;
}

uint8_t net_driver_debug(net_driver_t driver) {
    return driver->m_debug;
}

void net_driver_set_debug(net_driver_t driver, uint8_t debug) {
    driver->m_debug = debug;
}

void * net_driver_data(net_driver_t driver) {
    return driver + 1;
}

net_driver_t net_driver_from_data(void * data) {
    return ((net_driver_t)data) - 1;
}

net_driver_init_fun_t net_driver_init_fun(net_driver_t driver) {
    return driver->m_driver_init;
}


