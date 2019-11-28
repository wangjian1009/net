#ifndef NET_LIBEVENT_TIMER_H_INCLEDED
#define NET_LIBEVENT_TIMER_H_INCLEDED
#include "event2/event_struct.h"
#include "net_libevent_driver_i.h"

struct net_libevent_timer {
    struct event m_timer;
};

int net_libevent_timer_init(net_timer_t base_timer);
void net_libevent_timer_fini(net_timer_t base_timer);
void net_libevent_timer_active(net_timer_t base_timer, uint64_t delay_milliseconds);
void net_libevent_timer_cancel(net_timer_t base_timer);
uint8_t net_libevent_timer_is_active(net_timer_t base_timer);

#endif
