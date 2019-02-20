#ifndef NET_DNS_QUERY_H_INCLEDED
#define NET_DNS_QUERY_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

net_dns_query_t net_dns_query_create(
    net_schedule_t schedule, const char * hostname, const char * policy,
    net_dns_query_callback_fun_t callback, net_dns_query_ctx_free_fun_t ctx_free, void * ctx);

void net_dns_query_free(net_dns_query_t query);

void net_dns_query_free_by_ctx(net_schedule_t schedule, void * ctx);

void * net_dns_query_data(net_dns_query_t query);
net_dns_query_t net_dns_query_from_data(void * data);

void net_dns_query_notify_result_and_free(net_dns_query_t query, net_address_t main_address, net_address_it_t all_address);

NET_END_DECL

#endif
