#ifndef NET_TIMER_I_H_INCLEDED
#define NET_TIMER_I_H_INCLEDED
#include "net_timer.h"
#include "net_schedule_i.h"

NET_BEGIN_DECL

struct net_timer {
    net_driver_t m_driver;
    TAILQ_ENTRY(net_timer) m_next_for_driver;
    net_timer_process_fun_t m_process_fun;
    void * m_process_ctx;
};

void net_timer_real_free(net_timer_t timer);

NET_END_DECL

#endif
