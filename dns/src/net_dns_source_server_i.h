#ifndef NET_DNS_SOURCE_SERVER_I_H_INCLEDED
#define NET_DNS_SOURCE_SERVER_I_H_INCLEDED
#include "net_dns_source.h"
#include "net_dns_manage_i.h"

NET_BEGIN_DECL

struct net_dns_source_server {
    net_endpoint_t m_endpoint;
};

int net_dns_source_server_init(net_dns_source_t source);
void net_dns_source_server_fini(net_dns_source_t source);
void net_dns_source_server_dump(write_stream_t ws, net_dns_source_t source);

int net_dns_source_server_ctx_init(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
void net_dns_source_server_ctx_fini(net_dns_source_t source, net_dns_task_ctx_t task_ctx);

NET_END_DECL

#endif
