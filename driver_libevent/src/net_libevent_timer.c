#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "net_timer.h"
#include "net_libevent_timer.h"

int net_libevent_timer_init(net_timer_t base_timer) {
    /* net_libevent_timer_t timer = net_timer_data(base_timer); */

    /* bzero(&timer->m_watcher, sizeof(timer->m_watcher)); */
    /* timer->m_watcher.data = timer; */
    /* assert(!ev_is_active(&timer->m_watcher)); */
    return 0;
}

void net_libevent_timer_fini(net_timer_t base_timer) {
    /* net_libevent_timer_t timer = net_timer_data(base_timer); */
    /* net_libevent_driver_t driver = net_driver_data(net_timer_driver(base_timer)); */
    /* ev_timer_stop(driver->m_libevent_loop, &timer->m_watcher); */
}

void net_libevent_timer_active(net_timer_t base_timer, uint64_t delay_milliseconds) {
    /* net_libevent_timer_t timer = net_timer_data(base_timer); */
    /* net_libevent_driver_t driver = net_driver_data(net_timer_driver(base_timer)); */

    /* ev_timer_stop(driver->m_libevent_loop, &timer->m_watcher); */
    /* ev_timer_init(&timer->m_watcher, net_libevent_timer_cb, ((double)delay_milliseconds / 1000000.0), 0.0f); */
    /* ev_timer_start(driver->m_libevent_loop, &timer->m_watcher); */
    /* assert(ev_is_active(&timer->m_watcher)); */
}

void net_libevent_timer_cancel(net_timer_t base_timer) {
    /* net_libevent_timer_t timer = net_timer_data(base_timer); */
    /* net_libevent_driver_t driver = net_driver_data(net_timer_driver(base_timer)); */
    /* ev_timer_stop(driver->m_libevent_loop, &timer->m_watcher); */
}

uint8_t net_libevent_timer_is_active(net_timer_t base_timer) {
    /* net_libevent_timer_t timer = net_timer_data(base_timer); */
    /* return ev_is_active(&timer->m_watcher) || ev_is_pending(&timer->m_watcher); */
    return 0;
}
