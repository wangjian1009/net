#ifndef NET_KCP_DRIVER_I_H_INCLEDED
#define NET_KCP_DRIVER_I_H_INCLEDED
#include "cpe/utils/error.h"
#include "cpe/utils/memory.h"
#include "net_kcp_driver.h"

struct net_kcp_driver {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
};

mem_buffer_t net_kcp_driver_tmp_buffer(net_kcp_driver_t driver);

#endif
