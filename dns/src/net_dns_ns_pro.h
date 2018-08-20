#ifndef NET_DNS_NS_I_H_INCLEDED
#define NET_DNS_NS_I_H_INCLEDED
#include "net_dns_manage_i.h"

NET_BEGIN_DECL

char const * net_dns_ns_req_print_name(
    net_dns_manage_t manage, write_stream_t ws, char const * p, char const * buf, uint32_t buf_size, uint8_t level);
void net_dns_ns_req_print(net_dns_manage_t manage, write_stream_t ws, char const * buf, uint32_t buf_size);
const char * net_dns_ns_req_dump(net_dns_manage_t manage, mem_buffer_t buffer, char const * buf, uint32_t data_size);

NET_END_DECL

#endif
