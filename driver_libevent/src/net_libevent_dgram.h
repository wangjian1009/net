#ifndef NET_LIBEVENT_DGRAM_H_INCLEDED
#define NET_LIBEVENT_DGRAM_H_INCLEDED
#include "event2/event_struct.h"
#include "net_libevent_driver_i.h"

struct net_libevent_dgram {
    int m_domain;
    struct event m_event;
};

int net_libevent_dgram_init(net_dgram_t base_dgram);
void net_libevent_dgram_fini(net_dgram_t base_dgram);
int net_libevent_dgram_send(net_dgram_t base_dgram, net_address_t target, void const * data, size_t data_len);

#endif
