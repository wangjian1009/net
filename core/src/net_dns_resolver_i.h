#ifndef NET_DNS_RESOLVER_I_H_INCLEDED
#define NET_DNS_RESOLVER_I_H_INCLEDED
#include "net_dns_resolver.h"
#include "net_schedule_i.h"

NET_BEGIN_DECL

struct net_dns_resolver {
    net_schedule_t m_schedule;
    net_dns_mode_t m_mode;
    net_dns_server_list_t m_servers;
    net_dgram_t m_dgram;
    uint32_t m_max_query_id;
    struct cpe_hash_table m_querys;
};

NET_END_DECL

#endif
