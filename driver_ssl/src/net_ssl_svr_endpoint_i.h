#ifndef NET_SSL_SVR_ENDPOINT_I_H_INCLEDED
#define NET_SSL_SVR_ENDPOINT_I_H_INCLEDED
#include "net_ssl_svr_driver_i.h"

enum net_ssl_svr_endpoint_ssl_state {
    net_ssl_svr_endpoint_ssl_handshake,
    net_ssl_svr_endpoint_ssl_established,
};

struct net_ssl_svr_endpoint {
    net_endpoint_t m_underline;
    net_ssl_svr_endpoint_ssl_state_t m_state;
	SSL * m_ssl;
};

int net_ssl_svr_endpoint_init(net_endpoint_t endpoint);
void net_ssl_svr_endpoint_fini(net_endpoint_t endpoint);
int net_ssl_svr_endpoint_connect(net_endpoint_t endpoint);
void net_ssl_svr_endpoint_close(net_endpoint_t endpoint);
int net_ssl_svr_endpoint_update(net_endpoint_t endpoint);
int net_ssl_svr_endpoint_set_no_delay(net_endpoint_t endpoint, uint8_t no_delay);
int net_ssl_svr_endpoint_get_mss(net_endpoint_t endpoint, uint32_t * mss);

void net_ssl_svr_endpoint_dump_error(net_endpoint_t base_endpoint, int val);

#endif
