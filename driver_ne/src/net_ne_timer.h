#ifndef NET_NE_TIMER_H_INCLEDED
#define NET_NE_TIMER_H_INCLEDED
#include "cpe/pal/pal_socket.h"
#include "net_ne_driver_i.h"

struct net_ne_timer {
    //struct ev_timer m_watcher;
};

int net_ne_timer_init(net_timer_t base_timer);
void net_ne_timer_fini(net_timer_t base_timer);
void net_ne_timer_active(net_timer_t base_timer, uint32_t delay_ms);
void net_ne_timer_cancel(net_timer_t base_timer);
uint8_t net_ne_timer_is_active(net_timer_t base_timer);

#endif
