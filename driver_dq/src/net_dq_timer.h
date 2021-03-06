#ifndef NET_NE_TIMER_H_INCLEDED
#define NET_NE_TIMER_H_INCLEDED
#include "cpe/pal/pal_socket.h"
#include "net_dq_driver_i.h"

struct net_dq_timer {
    dispatch_source_t m_timer;
};

int net_dq_timer_init(net_timer_t base_timer);
void net_dq_timer_fini(net_timer_t base_timer);
void net_dq_timer_active(net_timer_t base_timer, uint64_t delay_milliseconds);
void net_dq_timer_cancel(net_timer_t base_timer);
uint8_t net_dq_timer_is_active(net_timer_t base_timer);

#endif
