#ifndef NET_ICMP_PING_RECORD_H_INCLEDED
#define NET_ICMP_PING_RECORD_H_INCLEDED
#include "net_icmp_types.h"

NET_BEGIN_DECL

struct net_icmp_ping_record_it {
    net_icmp_ping_record_t (*next)(net_icmp_ping_record_it_t it);
    char data[64];
};

#define net_icmp_ping_record_it_next(__it) ((__it)->next(__it))

NET_END_DECL

#endif
