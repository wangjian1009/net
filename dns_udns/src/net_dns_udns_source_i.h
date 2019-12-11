#ifndef NET_DNS_UDNS_SOURCE_I_H_INCLEDED
#define NET_DNS_UDNS_SOURCE_I_H_INCLEDED
#include "cpe/utils/error.h"
#include "cpe/utils/memory.h"
#include "net_dns_udns_source.h"

NET_BEGIN_DECL

struct dns_ctx;
struct dns_query;

struct net_dns_udns_source {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    uint8_t m_debug;
    net_dns_manage_t m_manage;
    net_driver_t m_driver;
    struct dns_ctx * m_dns_ctx;
    net_watcher_t m_watcher;
    net_timer_t m_timeout;
};

NET_END_DECL

#endif
