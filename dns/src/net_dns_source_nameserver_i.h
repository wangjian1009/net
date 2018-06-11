#ifndef NET_DNS_SOURCE_SERVER_I_H_INCLEDED
#define NET_DNS_SOURCE_SERVER_I_H_INCLEDED
#include "net_dns_source.h"
#include "net_dns_manage_i.h"

NET_BEGIN_DECL

struct net_dns_source_nameserver {
    net_address_t m_address;
    net_endpoint_t m_endpoint;
};

int net_dns_source_nameserver_build_req(
    net_dns_manage_t manage, mem_buffer_t buffer, net_address_t address,
    net_dns_task_ctx_t task_ctx);

NET_END_DECL

#endif
