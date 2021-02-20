#ifndef NET_KCP_ACCEPTOR_H_INCLEDED
#define NET_KCP_ACCEPTOR_H_INCLEDED
#include "net_kcp_types.h"

NET_BEGIN_DECL

net_kcp_acceptor_t net_kcp_acceptor_cast(net_acceptor_t base_acceptor);
int net_kcp_acceptor_enable(net_kcp_acceptor_t, net_kcp_config_t config);

NET_END_DECL

#endif
