#ifndef NET_DNS_SCOPE_I_H_INCLEDED
#define NET_DNS_SCOPE_I_H_INCLEDED
#include "net_dns_scope.h"
#include "net_dns_manage_i.h"

NET_BEGIN_DECL

struct net_dns_scope {
    net_dns_manage_t m_manage;
    struct cpe_hash_entry m_hh;
    const char * m_name;
};

void net_dns_scope_free_all(net_dns_manage_t manage);

uint32_t net_dns_scope_hash(net_dns_scope_t o, void * user_data);
int net_dns_scope_eq(net_dns_scope_t l, net_dns_scope_t r, void * user_data);

NET_END_DECL

#endif
