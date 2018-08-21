#ifndef NET_DNS_NS_CLI_DGRAM_I_H_INCLEDED
#define NET_DNS_NS_CLI_DGRAM_I_H_INCLEDED
#include "net_dns_manage_i.h"

NET_BEGIN_DECL

void net_dns_ns_cli_dgram_input(net_dgram_t dgram, void * ctx, void * data, size_t data_size, net_address_t from);

NET_END_DECL

#endif
