#ifndef NET_SOCK_ENDPOINT_H_INCLEDED
#define NET_SOCK_ENDPOINT_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_sock_types.h"

NET_BEGIN_DECL

net_sock_endpoint_t net_sock_endpoint_from_base_endpoint(net_endpoint_t base_ep);
net_endpoint_t net_sock_endpoint_base_endpoint(net_sock_endpoint_t endpoint);

int net_sock_endpoint_fd(net_sock_endpoint_t endpoint);
int net_sock_endpoint_set_dft_block_size_to_mss(net_sock_endpoint_t endpoint);
int net_sock_endpoint_set_no_delay(net_sock_endpoint_t endpoint, uint8_t is_enable);

NET_END_DECL

#endif
