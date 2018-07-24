#ifndef NET_NE_DRIVER_I_H_INCLEDED
#define NET_NE_DRIVER_I_H_INCLEDED
#import <NetworkExtension/NETunnelProvider.h>
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_ne_driver.h"

typedef TAILQ_HEAD(net_ne_acceptor_list, net_ne_acceptor) net_ne_acceptor_list_t;

typedef struct net_ne_endpoint * net_ne_endpoint_t;
typedef struct net_ne_dgram * net_ne_dgram_t;
typedef struct net_ne_timer * net_ne_timer_t;

struct net_ne_driver {
    __unsafe_unretained NETunnelProvider * m_tunnel_provider;
    uint8_t m_debug;
    net_ne_driver_sock_create_process_fun_t m_sock_process_fun;
    void * m_sock_process_ctx;
    net_data_monitor_fun_t m_data_monitor_fun;
    void * m_data_monitor_ctx;
    
    net_ne_acceptor_list_t m_acceptors;
};

mem_buffer_t net_ne_driver_tmp_buffer(net_ne_driver_t driver);

#endif
