#ifndef NET_DNS_UDNS_SOURCE_CTX_I_H_INCLEDED
#define NET_DNS_UDNS_SOURCE_CTX_I_H_INCLEDED
#include "net_dns_udns_source_i.h"

NET_BEGIN_DECL

struct net_dns_udns_source_ctx {
    int n;
};

int net_dns_udns_source_ctx_init(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
void net_dns_udns_source_ctx_fini(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
int net_dns_udns_source_ctx_start(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
void net_dns_udns_source_ctx_cancel(net_dns_source_t source, net_dns_task_ctx_t task_ctx);

NET_END_DECL

#endif
