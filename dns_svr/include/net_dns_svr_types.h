#ifndef NET_DNS_SVR_TYPES_H_INCLEDED
#define NET_DNS_SVR_TYPES_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef struct net_dns_svr * net_dns_svr_t;
typedef struct net_dns_svr_itf * net_dns_svr_itf_t;
typedef struct net_dns_svr_monitor * net_dns_svr_monitor_t;
typedef struct net_dns_svr_query * net_dns_svr_query_t;

typedef enum net_dns_svr_itf_type {
    net_dns_svr_itf_udp,
    net_dns_svr_itf_tcp,
} net_dns_svr_itf_type_t;

typedef enum net_dns_svr_query_error {
    net_dns_svr_query_error_none,
    net_dns_svr_query_error_internal
} net_dns_svr_query_error_t;

NET_END_DECL

#endif

