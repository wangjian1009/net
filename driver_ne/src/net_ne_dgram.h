#ifndef NET_NE_DGRAM_H_INCLEDED
#define NET_NE_DGRAM_H_INCLEDED
#import "NetworkExtension/NWUDPSession.h"
#include "net_ne_driver_i.h"

struct net_ne_dgram {
    struct cpe_hash_table m_sessions;
};

int net_ne_dgram_init(net_dgram_t base_dgram);
void net_ne_dgram_fini(net_dgram_t base_dgram);
int net_ne_dgram_send(net_dgram_t base_dgram, net_address_t target, void const * data, size_t data_len);

net_ne_driver_t net_ne_dgram_driver(net_ne_dgram_t dgram);

#endif
