#ifndef NET_DNS_CLI_PROTOCOL_I_H_INCLEDED
#define NET_DNS_CLI_PROTOCOL_I_H_INCLEDED
#include "net_dns_manage_i.h"

NET_BEGIN_DECL

struct net_dns_cli_protocol {
    net_dns_manage_t m_manage;
};

net_protocol_t net_dns_cli_protocol_create(net_dns_manage_t manage);

NET_END_DECL

#endif
