#ifndef NET_DQ_DGRAM_H_INCLEDED
#define NET_DQ_DGRAM_H_INCLEDED
#include "cpe/pal/pal_socket.h"
#include "net_dq_driver_i.h"

struct net_dq_dgram {
    int m_fd;
    __unsafe_unretained dispatch_source_t m_source_r;
};

int net_dq_dgram_init(net_dgram_t base_dgram);
void net_dq_dgram_fini(net_dgram_t base_dgram);
int net_dq_dgram_send(net_dgram_t base_dgram, net_address_t target, void const * data, size_t data_len);

#endif
