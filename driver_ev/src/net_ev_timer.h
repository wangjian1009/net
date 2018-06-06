#ifndef NET_EV_TIMER_H_INCLEDED
#define NET_EV_TIMER_H_INCLEDED
#include "cpe/pal/pal_socket.h"
#include "net_ev_driver_i.h"

struct net_ev_timer {
    struct ev_timer m_watcher;
};

int net_ev_timer_init(net_timer_t base_timer);
void net_ev_timer_fini(net_timer_t base_timer);
void net_ev_timer_active(net_timer_t base_timer, int32_t delay_ms);
void net_ev_timer_cancel(net_timer_t base_timer);
uint8_t net_ev_timer_is_active(net_timer_t base_timer);

#endif
