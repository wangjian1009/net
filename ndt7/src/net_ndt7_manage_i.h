#ifndef NET_NDT_MANAGE_I_H_INCLEDED
#define NET_NDT_MANAGE_I_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "cpe/utils/buffer.h"
#include "net_ndt_manage.h"

NET_BEGIN_DECL

struct net_ndt_manage {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    uint8_t m_debug;
    uint32_t m_cfg_ttl_s;
    net_schedule_t m_schedule;
};

mem_buffer_t net_ndt_manage_tmp_buffer(net_ndt_manage_t manage);
int net_ndt_manage_active_delay_process(net_ndt_manage_t manage);

NET_END_DECL

#endif
