#ifndef NET_ANDROID_TIMER_H_INCLEDED
#define NET_ANDROID_TIMER_H_INCLEDED
#include "cpe/pal/pal_socket.h"
#include "net_android_driver_i.h"

struct net_android_timer {
    struct ev_timer m_watcher;
};

int net_android_timer_init(net_timer_t base_timer);
void net_android_timer_fini(net_timer_t base_timer);
void net_android_timer_active(net_timer_t base_timer, uint64_t delay_milliseconds);
void net_android_timer_cancel(net_timer_t base_timer);
uint8_t net_android_timer_is_active(net_timer_t base_timer);

#endif
