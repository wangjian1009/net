#ifndef NET_DNS_SCOPE_H_INCLEDED
#define NET_DNS_SCOPE_H_INCLEDED
#include "net_dns_system.h"

NET_BEGIN_DECL

struct net_dns_scope_it {
    net_dns_scope_t (*next)(net_dns_scope_it_t it);
    char data[64];
};

net_dns_scope_t net_dns_scope_create(net_dns_manage_t manage, const char * name);
void net_dns_scope_free(net_dns_scope_t dns_scope);

net_dns_scope_t net_dns_scope_find(net_dns_manage_t manage, const char * name);
void net_dns_scopes(net_dns_manage_t manage, net_dns_scope_it_t it);

const char * net_dns_scope_name(net_dns_scope_t scope);
void net_dns_scope_clear(net_dns_scope_t scope);

int net_dns_scope_add_source(net_dns_scope_t scope, net_dns_source_t source);
uint8_t net_dns_scope_have_source(net_dns_scope_t scope, net_dns_source_t source);

#define net_dns_scope_it_next(__it) ((__it)->next(__it))

NET_END_DECL

#endif
