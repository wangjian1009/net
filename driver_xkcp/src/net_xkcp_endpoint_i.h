#ifndef NET_XKCP_ENDPOINT_I_H_INCLEDED
#define NET_XKCP_ENDPOINT_I_H_INCLEDED
#include "net_xkcp_endpoint.h"
#include "net_xkcp_driver_i.h"

struct net_xkcp_endpoint {
    net_endpoint_t m_base_endpoint;
    ikcpcb * m_kcp;
	iqueue_head m_head;
};

int net_xkcp_endpoint_init(net_endpoint_t endpoint);
void net_xkcp_endpoint_fini(net_endpoint_t endpoint);
int net_xkcp_endpoint_connect(net_endpoint_t endpoint);
int net_xkcp_endpoint_update(net_endpoint_t endpoint);
int net_xkcp_endpoint_set_no_delay(net_endpoint_t endpoint, uint8_t no_delay);
int net_xkcp_endpoint_get_mss(net_endpoint_t endpoint, uint32_t * mss);

#endif
