#ifndef NET_NE_DGRAM_H_INCLEDED
#define NET_NE_DGRAM_H_INCLEDED
#import "NetworkExtension/NWUDPSession.h"
#include "net_ne_driver_i.h"

struct net_ne_dgram {
    __unsafe_unretained NWUDPSession* m_session;
    /* private var pendingWriteData: [Data] = [] */
    /* private var writing = false */
    /* private let queue: DispatchQueue = QueueFactory.getQueue() */
    /* private let timer: DispatchSourceTimer */
    /* private let timeout: Int */
};

int net_ne_dgram_init(net_dgram_t base_dgram);
void net_ne_dgram_fini(net_dgram_t base_dgram);
int net_ne_dgram_send(net_dgram_t base_dgram, net_address_t target, void const * data, size_t data_len);

#endif
