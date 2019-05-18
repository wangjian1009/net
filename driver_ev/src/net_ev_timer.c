#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "net_timer.h"
#include "net_sock_driver.h"
#include "net_ev_timer.h"

static void net_ev_timer_cb(EV_P_ ev_timer *watcher, int revents);

int net_ev_timer_init(net_timer_t base_timer) {
    net_ev_timer_t timer = net_timer_data(base_timer);

    bzero(&timer->m_watcher, sizeof(timer->m_watcher));
    timer->m_watcher.data = timer;
    return 0;
}

void net_ev_timer_fini(net_timer_t base_timer) {
    net_ev_timer_t timer = net_timer_data(base_timer);
    net_ev_driver_t driver = net_sock_driver_data_from_base_driver(net_timer_driver(base_timer));
    ev_timer_stop(driver->m_ev_loop, &timer->m_watcher);
}

void net_ev_timer_active(net_timer_t base_timer, uint32_t delay_ms) {
    net_ev_timer_t timer = net_timer_data(base_timer);
    net_ev_driver_t driver = net_sock_driver_data_from_base_driver(net_timer_driver(base_timer));

    ev_timer_stop(driver->m_ev_loop, &timer->m_watcher);
    ev_timer_init(&timer->m_watcher, net_ev_timer_cb, ((double)delay_ms / 1000.0), 0.0f);
    ev_timer_start(driver->m_ev_loop, &timer->m_watcher);
}

void net_ev_timer_cancel(net_timer_t base_timer) {
    net_ev_timer_t timer = net_timer_data(base_timer);
    if (!ev_is_active(&timer->m_watcher)) return;

    net_ev_driver_t driver = net_sock_driver_data_from_base_driver(net_timer_driver(base_timer));
    ev_timer_stop(driver->m_ev_loop, &timer->m_watcher);
}

uint8_t net_ev_timer_is_active(net_timer_t base_timer) {
    net_ev_timer_t timer = net_timer_data(base_timer);
    return ev_is_active(&timer->m_watcher);
}

static void net_ev_timer_cb(EV_P_ ev_timer *watcher, int revents) {
    net_ev_timer_t timer = watcher->data;
    net_timer_t base_timer = net_timer_from_data(timer);
    net_ev_driver_t driver = net_sock_driver_data_from_base_driver(net_timer_driver(base_timer));

    ev_timer_stop(driver->m_ev_loop, &timer->m_watcher);

    net_timer_process(base_timer);
}
