#ifndef NET_DNS_SVR_I_H_INCLEDED
#define NET_DNS_SVR_I_H_INCLEDED
#include "cpe/utils/memory.h"
#include "cpe/utils/error.h"
#include "net_dns_svr.h"

NET_BEGIN_DECL

struct net_dns_svr {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    uint8_t m_debug;
};

NET_END_DECL

#endif


