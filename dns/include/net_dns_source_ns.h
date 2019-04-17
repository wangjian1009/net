#ifndef NET_DNS_SOURCE_NS_H_INCLEDED
#define NET_DNS_SOURCE_NS_H_INCLEDED
#include "net_dns_system.h"

NET_BEGIN_DECL

net_dns_source_ns_t
net_dns_source_ns_create(
    net_dns_manage_t manage, net_driver_t driver,
    net_address_t addr, uint8_t is_own,
    net_dns_ns_trans_type_t trans_type, uint16_t timeout_s, uint16_t retry_count);

void net_dns_source_ns_free(net_dns_source_ns_t ns);

net_dns_source_ns_t net_dns_source_ns_from_source(net_dns_source_t source);

net_dns_source_ns_t net_dns_source_ns_find(net_dns_manage_t manage, net_address_t addr);

net_address_t net_dns_source_ns_address(net_dns_source_ns_t ns);

uint16_t net_dns_source_ns_timeout_s(net_dns_source_ns_t ns);
void net_dns_source_ns_set_timeout_s(net_dns_source_ns_t ns, uint16_t timeout_s);

net_dns_ns_trans_type_t net_dns_source_ns_trans_type(net_dns_source_ns_t ns);
void net_dns_source_ns_set_trans_type(net_dns_source_ns_t ns, net_dns_ns_trans_type_t trans_type);

void net_dns_source_ns_set_tcp_connect(
    net_dns_source_ns_t ns, net_endpoint_prepare_connect_fun_t tcp_connect, void * tcp_connect_ctx);

NET_END_DECL

#endif
