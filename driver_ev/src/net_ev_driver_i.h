#ifndef NET_DRIVER_EV_I_H_INCLEDED
#define NET_DRIVER_EV_I_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_ev_driver.h"

typedef struct net_ev_acceptor * net_ev_acceptor_t;
typedef struct net_ev_endpoint * net_ev_endpoint_t;
typedef struct net_ev_dgram * net_ev_dgram_t;
typedef struct net_ev_watcher * net_ev_watcher_t;
typedef struct net_ev_timer * net_ev_timer_t;

struct net_ev_driver {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    struct ev_loop * m_ev_loop;
    net_ev_driver_sock_create_process_fun_t m_sock_process_fun;
    void * m_sock_process_ctx;
    net_data_monitor_fun_t m_data_monitor_fun;
    void * m_data_monitor_ctx;
};

net_schedule_t net_ev_driver_schedule(net_ev_driver_t driver);
mem_buffer_t net_ev_driver_tmp_buffer(net_ev_driver_t driver);

#endif
