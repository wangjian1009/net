#ifndef NET_XKCP_DRIVER_I_H_INCLEDED
#define NET_XKCP_DRIVER_I_H_INCLEDED
#include "ikcp.h"
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/hash.h"
#include "cpe/utils/error.h"
#include "cpe/utils/memory.h"
#include "net_xkcp_driver.h"

typedef struct net_xkcp_dgram * net_xkcp_dgram_t;

struct net_xkcp_driver {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    net_driver_t m_underline_driver;
    struct cpe_hash_table m_connectors;
};

mem_buffer_t net_xkcp_driver_tmp_buffer(net_xkcp_driver_t driver);

#endif
