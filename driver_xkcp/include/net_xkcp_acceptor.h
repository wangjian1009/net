#ifndef NET_XKCP_ACCEPTOR_H_INCLEDED
#define NET_XKCP_ACCEPTOR_H_INCLEDED
#include "net_xkcp_types.h"

NET_BEGIN_DECL

net_xkcp_acceptor_t net_xkcp_acceptor_cast(net_acceptor_t base_acceptor);

int net_xkcp_acceptor_set_config(net_xkcp_acceptor_t acceptor, net_xkcp_config_t config);
net_xkcp_config_t net_xkcp_acceptor_config(net_xkcp_acceptor_t acceptor);

net_dgram_t net_xkcp_acceptor_dgram(net_xkcp_acceptor_t acceptor);

net_xkcp_client_t
net_xkcp_acceptor_find_client(net_xkcp_acceptor_t acceptor, net_address_t remote_address);

NET_END_DECL

#endif
