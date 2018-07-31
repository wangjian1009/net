#ifndef NET_NE_DRIVER_I_H_INCLEDED
#define NET_NE_DRIVER_I_H_INCLEDED
#include <dispatch/source.h>
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "cpe/utils/hash.h"
#include "net_schedule.h"
#include "net_dq_driver.h"

typedef struct net_dq_endpoint * net_dq_endpoint_t;
typedef struct net_dq_dgram * net_dq_dgram_t;
typedef struct net_dq_timer * net_dq_timer_t;

struct net_dq_driver {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    uint8_t m_debug;
    net_data_monitor_fun_t m_data_monitor_fun;
    void * m_data_monitor_ctx;
};

net_schedule_t net_dq_driver_schedule(net_dq_driver_t driver);
mem_buffer_t net_dq_driver_tmp_buffer(net_dq_driver_t driver);

#endif
