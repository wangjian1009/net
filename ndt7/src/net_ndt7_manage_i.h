#ifndef NET_NDT7_MANAGE_I_H_INCLEDED
#define NET_NDT7_MANAGE_I_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "cpe/utils/buffer.h"
#include "net_http_types.h"
#include "net_ssl_types.h"
#include "net_ndt7_manage.h"

NET_BEGIN_DECL

typedef TAILQ_HEAD(net_ndt7_tester_list, net_ndt7_tester) net_ndt7_tester_list_t;

struct net_ndt7_manage {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    uint8_t m_debug;
    net_schedule_t m_schedule;
    net_driver_t m_base_driver;
    net_driver_t m_ssl_driver;
    net_http_protocol_t m_http_protocol;
    uint32_t m_idx_max;
    net_ndt7_tester_list_t m_testers;
    net_ndt7_tester_list_t m_to_notify_testers;
    net_timer_t m_delay_process;
};

mem_buffer_t net_ndt7_manage_tmp_buffer(net_ndt7_manage_t manage);

NET_END_DECL

#endif
