#ifndef NET_KCP_ENDPOINT_I_H_INCLEDED
#define NET_KCP_ENDPOINT_I_H_INCLEDED
#include "net_kcp_endpoint.h"
#include "net_kcp_driver_i.h"

struct net_kcp_endpoint {
    net_endpoint_t m_base_endpoint;
};

int net_kcp_endpoint_init(net_endpoint_t endpoint);
void net_kcp_endpoint_fini(net_endpoint_t endpoint);
int net_kcp_endpoint_connect(net_endpoint_t endpoint);
int net_kcp_endpoint_update(net_endpoint_t endpoint);
int net_kcp_endpoint_set_no_delay(net_endpoint_t endpoint, uint8_t no_delay);
int net_kcp_endpoint_get_mss(net_endpoint_t endpoint, uint32_t * mss);

#endif
