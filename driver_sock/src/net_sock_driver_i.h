#ifndef NET_SOCK_DRIVER_I_H_INCLEDED
#define NET_SOCK_DRIVER_I_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_sock_driver.h"

typedef struct net_sock_acceptor * net_sock_acceptor_t;
typedef struct net_sock_dgram * net_sock_dgram_t;

struct net_sock_driver {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    net_sock_driver_init_fun_t m_init;
    net_sock_driver_fini_fun_t m_fini;
};

net_schedule_t net_sock_driver_schedule(net_sock_driver_t driver);
mem_buffer_t net_sock_driver_tmp_buffer(net_sock_driver_t driver);

#ifdef _WIN32
#define _to_watcher_fd(__fd) (_get_osfhandle((__fd)))
#else
#define _to_watcher_fd(__fd) ((__fd))
#endif

#endif
