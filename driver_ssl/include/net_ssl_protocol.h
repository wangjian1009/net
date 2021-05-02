#ifndef NET_SSL_PROTOCOL_H_INCLEDED
#define NET_SSL_PROTOCOL_H_INCLEDED
#include "net_ssl_types.h"

NET_BEGIN_DECL

net_ssl_protocol_t
net_ssl_protocol_create(
    net_schedule_t schedule, const char * addition_name, mem_allocrator_t alloc, error_monitor_t em);

void net_ssl_protocol_free(net_ssl_protocol_t protocol);

net_ssl_protocol_t
net_ssl_protocol_find(net_schedule_t schedule, const char * addition_name);

net_ssl_protocol_t net_ssl_protocol_cast(net_protocol_t protocol);

void net_ssl_protocol_print_supported_ciphersuites(write_stream_t ws, net_ssl_protocol_t protocol);
const char * net_ssl_protocol_dump_supported_ciphersuites(mem_buffer_t buffer, net_ssl_protocol_t protocol);

int net_ssl_protocol_set_ciphersuites(net_ssl_protocol_t protocol, const char * ciphersuites);
int net_ssl_protocol_add_ciphersuite(net_ssl_protocol_t protocol, const char * ciphersuite);
int net_ssl_protocol_set_ciphersuites_all(net_ssl_protocol_t protocol);

int net_ssl_protocol_svr_confirm_pkey(net_ssl_protocol_t protocol);
int net_ssl_protocol_svr_confirm_cert(net_ssl_protocol_t protocol);
int net_ssl_protocol_svr_prepaired(net_ssl_protocol_t protocol);

NET_END_DECL

#endif
