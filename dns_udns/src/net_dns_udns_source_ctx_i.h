#ifndef NET_DNS_UDNS_SOURCE_CTX_I_H_INCLEDED
#define NET_DNS_UDNS_SOURCE_CTX_I_H_INCLEDED
#include "net_dns_udns_source_i.h"

NET_BEGIN_DECL

typedef enum net_dns_udns_source_query_type {
    net_dns_udns_source_query_type_ipv4,
    net_dns_udns_source_query_type_ipv6,
} net_dns_udns_source_query_type_t;

struct net_dns_udns_source_ctx {
    TAILQ_ENTRY(net_dns_udns_source_ctx) m_next;
    struct dns_query * m_queries[2];
    uint32_t m_result_count;
};

int net_dns_udns_source_ctx_init(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
void net_dns_udns_source_ctx_fini(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
int net_dns_udns_source_ctx_start(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
void net_dns_udns_source_ctx_cancel(net_dns_source_t source, net_dns_task_ctx_t task_ctx);

void net_dns_udns_source_ctx_active_cancel(net_dns_udns_source_ctx_t udns_ctx);

NET_END_DECL

#endif
