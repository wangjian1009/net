#ifndef NET_XKCP_ACCEPTOR_H_INCLEDED
#define NET_XKCP_ACCEPTOR_H_INCLEDED
#include "net_xkcp_types.h"

NET_BEGIN_DECL

net_xkcp_acceptor_t net_xkcp_acceptor_cast(net_acceptor_t base_acceptor);
int net_xkcp_acceptor_enable(net_xkcp_acceptor_t, net_xkcp_config_t config);

NET_END_DECL

#endif
