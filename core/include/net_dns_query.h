#ifndef NET_DNS_QUERY_H_INCLEDED
#define NET_DNS_QUERY_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

net_dns_query_t net_dns_query_create(
    net_dns_resolver_t resolver, const char *hostname,
    net_dns_query_callback_fun_t callback, net_dns_query_ctx_free_fun_t ctx_free, void * ctx);

void net_dns_query_free(net_dns_query_t query);

NET_END_DECL

#endif
