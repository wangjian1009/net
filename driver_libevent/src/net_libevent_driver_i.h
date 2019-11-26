#ifndef NET_DRIVER_LIBEVENT_I_H_INCLEDED
#define NET_DRIVER_LIBEVENT_I_H_INCLEDED
#include <event2/event.h>
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "net_schedule.h"
#include "net_libevent_driver.h"

typedef struct net_libevent_acceptor * net_libevent_acceptor_t;
typedef struct net_libevent_endpoint * net_libevent_endpoint_t;
typedef struct net_libevent_dgram * net_libevent_dgram_t;
typedef struct net_libevent_watcher * net_libevent_watcher_t;
typedef struct net_libevent_timer * net_libevent_timer_t;

struct net_libevent_driver {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    struct event_base * m_event_base;
};

net_schedule_t net_libevent_driver_schedule(net_libevent_driver_t driver);
mem_buffer_t net_libevent_driver_tmp_buffer(net_libevent_driver_t driver);

#endif
