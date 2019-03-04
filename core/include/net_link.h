#ifndef NET_LINK_H_INCLEDED
#define NET_LINK_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

net_link_t
net_link_create(
    net_endpoint_t local, uint8_t local_is_tie, net_endpoint_t remote, uint8_t remote_is_tie);

void net_link_free(net_link_t link);

uint8_t net_link_is_tie(net_link_t link, net_endpoint_t endpoint);
void net_link_set_tie(net_link_t link, net_endpoint_t endpoint, uint8_t is_tie);

uint32_t net_link_buf_limit(net_link_t link);
int net_link_set_buf_limit(net_link_t link, uint32_t limit);

NET_END_DECL

#endif
