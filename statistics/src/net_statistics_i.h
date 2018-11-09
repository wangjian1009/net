#ifndef NET_CONTROL_I_H_INCLEDED
#define NET_CONTROL_I_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "cpe/utils/buffer.h"
#include "cpe/utils/hash.h"
#include "net_statistics.h"

CPE_BEGIN_DECL

typedef TAILQ_HEAD(net_statistics_reportor_type_list, net_statistics_reportor_type) net_statistics_reportor_type_list_t;
typedef TAILQ_HEAD(net_statistics_reportor_list, net_statistics_reportor) net_statistics_reportor_list_t;

struct net_statistics {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    uint8_t m_debug;

    net_statistics_reportor_type_list_t m_reportor_types;
    net_statistics_reportor_list_t m_reportors;    
};

CPE_END_DECL

#endif

