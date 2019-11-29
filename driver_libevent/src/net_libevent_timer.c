#include "assert.h"
#include "cpe/pal/pal_string.h"
#include "cpe/pal/pal_strings.h"
#include "net_timer.h"
#include "net_driver.h"
#include "net_sock_driver.h"
#include "net_libevent_timer.h"

void net_libevent_timer_cb(int fd, short events, void* arg);

int net_libevent_timer_init(net_timer_t base_timer) {
    net_libevent_timer_t timer = net_timer_data(base_timer);
    net_libevent_driver_t driver = net_sock_driver_data_from_base_driver(net_timer_driver(base_timer));

    bzero(&timer->m_timer, sizeof(timer->m_timer));

    return 0;
}

void net_libevent_timer_fini(net_timer_t base_timer) {
    net_libevent_timer_t timer = net_timer_data(base_timer);

    if (evtimer_initialized(&timer->m_timer)) {
        evtimer_del(&timer->m_timer);
    }
}

void net_libevent_timer_active(net_timer_t base_timer, uint64_t delay_milliseconds) {
    net_libevent_timer_t timer = net_timer_data(base_timer);
    net_libevent_driver_t driver = net_sock_driver_data_from_base_driver(net_timer_driver(base_timer));

    if (evtimer_initialized(&timer->m_timer)) {
        evtimer_del(&timer->m_timer);
    }

    evtimer_assign(&timer->m_timer, driver->m_event_base, net_libevent_timer_cb, base_timer);

    struct timeval tv;
    tv.tv_sec = delay_milliseconds / 1000;
    tv.tv_usec = delay_milliseconds - ((uint64_t)tv.tv_sec * 100U);
    evtimer_add(&timer->m_timer, &tv);
}

void net_libevent_timer_cancel(net_timer_t base_timer) {
    net_libevent_timer_t timer = net_timer_data(base_timer);

    if (evtimer_initialized(&timer->m_timer)) {
        evtimer_del(&timer->m_timer);
    }
}

uint8_t net_libevent_timer_is_active(net_timer_t base_timer) {
    net_libevent_timer_t timer = net_timer_data(base_timer);
    return evtimer_initialized(&timer->m_timer);
}

void net_libevent_timer_cb(int fd, short events, void* arg) {
    net_timer_t base_timer = arg;
    net_timer_process(base_timer);
}
