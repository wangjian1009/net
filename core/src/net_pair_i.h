#ifndef NET_CORE_PAIR_I_H_INCLEDED
#define NET_CORE_PAIR_I_H_INCLEDED
#include "net_pair.h"
#include "net_schedule_i.h"

typedef struct net_pair_endpoint * net_pair_endpoint_t;

struct net_pair_endpoint {
    net_pair_endpoint_t m_other;
    net_timer_t m_delay_processor;
    uint8_t m_is_writing;
    uint8_t m_is_state_bind;
};

net_driver_t net_pair_driver_create(net_schedule_t schedule);

#endif
