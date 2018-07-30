#ifndef NET_NE_DRIVER_I_H_INCLEDED
#define NET_NE_DRIVER_I_H_INCLEDED
#import <NetworkExtension/NETunnelProvider.h>
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "cpe/utils/hash.h"
#include "net_schedule.h"
#include "net_ne_driver.h"

typedef TAILQ_HEAD(net_ne_acceptor_list, net_ne_acceptor) net_ne_acceptor_list_t;
typedef TAILQ_HEAD(net_ne_dgram_session_list, net_ne_dgram_session) net_ne_dgram_session_list_t;

typedef struct net_ne_endpoint * net_ne_endpoint_t;
typedef struct net_ne_dgram * net_ne_dgram_t;
typedef struct net_ne_dgram_session * net_ne_dgram_session_t;
typedef struct net_ne_timer * net_ne_timer_t;

struct net_ne_driver {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    __unsafe_unretained NETunnelProvider * m_tunnel_provider;
    uint8_t m_debug;
    net_data_monitor_fun_t m_data_monitor_fun;
    void * m_data_monitor_ctx;

    uint32_t m_dgram_max_session_id;
    struct cpe_hash_table m_dgram_sessions;
    ringbuffer_t m_dgram_data_buf;

    net_ne_acceptor_list_t m_acceptors;

    net_ne_dgram_session_list_t m_free_dgram_sessions;
};

net_schedule_t net_ne_driver_schedule(net_ne_driver_t driver);
mem_buffer_t net_ne_driver_tmp_buffer(net_ne_driver_t driver);

#endif
