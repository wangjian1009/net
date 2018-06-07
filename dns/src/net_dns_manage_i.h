#ifndef NET_DNS_MANAGE_I_H_INCLEDED
#define NET_DNS_MANAGE_I_H_INCLEDED
#include "cpe/pal/pal_queue.h"
#include "net_dns_manage.h"

NET_BEGIN_DECL

typedef TAILQ_HEAD(net_dns_server_list, net_dns_server) net_dns_server_list_t;

struct net_dns_manage {
    mem_allocrator_t m_alloc;
    error_monitor_t m_em;
    net_schedule_t m_schedule;
    net_dns_mode_t m_mode;
    net_dns_server_list_t m_servers;
    net_dgram_t m_dgram;
};

NET_END_DECL

#endif
