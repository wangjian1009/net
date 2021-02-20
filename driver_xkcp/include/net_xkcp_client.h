#ifndef NET_XKCP_CLIENT_H_INCLEDED
#define NET_XKCP_CLIENT_H_INCLEDED
#include "net_xkcp_types.h"

NET_BEGIN_DECL

net_xkcp_endpoint_t net_xkcp_client_find_stream(net_xkcp_client_t client, uint32_t conv);

NET_END_DECL

#endif
