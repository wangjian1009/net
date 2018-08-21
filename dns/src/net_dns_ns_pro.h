#ifndef NET_DNS_NS_PRO_H_INCLEDED
#define NET_DNS_NS_PRO_H_INCLEDED
#include "net_dns_manage_i.h"

NET_BEGIN_DECL

uint8_t const * net_dns_ns_req_print_name(
    net_dns_manage_t manage, write_stream_t ws, uint8_t const * p, uint8_t const * buf, uint32_t buf_size, uint8_t level);
void net_dns_ns_req_print(net_dns_manage_t manage, write_stream_t ws, uint8_t const * buf, uint32_t buf_size);
const char * net_dns_ns_req_dump(net_dns_manage_t manage, mem_buffer_t buffer, uint8_t const * buf, uint32_t data_size);

NET_END_DECL

#endif
