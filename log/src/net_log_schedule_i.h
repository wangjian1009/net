#ifndef NET_LOG_SCHEDULE_I_H_INCLEDED
#define NET_LOG_SCHEDULE_I_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/hash.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "cpe/utils/buffer.h"
#include "net_log_schedule.h"

NET_BEGIN_DECL

struct net_log_schedule {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    uint8_t m_debug;
    net_schedule_t m_net_schedule;
    net_driver_t m_net_driver;
};

mem_buffer_t net_log_schedule_tmp_buffer(net_log_schedule_t schedule);

NET_END_DECL

#endif
