#ifndef NET_PING_RECORD_H_INCLEDED
#define NET_PING_RECORD_H_INCLEDED
#include "net_ping_types.h"

NET_BEGIN_DECL

struct net_ping_record_it {
    net_ping_record_t (*next)(net_ping_record_it_t it);
    char data[64];
};

#define net_ping_record_it_next(__it) ((__it)->next(__it))

NET_END_DECL

#endif
