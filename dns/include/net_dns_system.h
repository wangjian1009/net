#ifndef NET_DNS_SYSTEM_H_INCLEDED
#define NET_DNS_SYSTEM_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef enum net_dns_mode {
    net_dns_ipv4_first,
    net_dns_ipv6_first,
} net_dns_mode_t;

typedef struct net_dns_manage * net_dns_manage_t;
typedef struct net_dns_server * net_dns_server_t;

NET_END_DECL

#endif
