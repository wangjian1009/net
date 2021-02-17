#ifndef NET_KCP_DRIVER_I_H_INCLEDED
#define NET_KCP_DRIVER_I_H_INCLEDED
#include "ikcp.h"
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/error.h"
#include "cpe/utils/memory.h"
#include "net_kcp_driver.h"

typedef struct net_kcp_mux * net_kcp_mux_t;
typedef TAILQ_HEAD(net_kcp_mux_list, net_kcp_mux) net_kcp_mux_list_t;
    
struct net_kcp_driver {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    net_driver_t m_underline_driver;
    net_kcp_mux_list_t m_muxes;
};

mem_buffer_t net_kcp_driver_tmp_buffer(net_kcp_driver_t driver);

#endif
