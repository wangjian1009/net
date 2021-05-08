#ifndef NET_SSL_STREAM_ENDPOINT_I_H_INCLEDED
#define NET_SSL_STREAM_ENDPOINT_I_H_INCLEDED
#include "net_ssl_stream_endpoint.h"
#include "net_ssl_stream_driver_i.h"

struct net_ssl_stream_endpoint {
    net_endpoint_t m_base_endpoint;
    net_ssl_endpoint_t m_underline;
};

int net_ssl_stream_endpoint_init(net_endpoint_t endpoint);
void net_ssl_stream_endpoint_fini(net_endpoint_t endpoint);
void net_ssl_stream_endpoint_calc_size(net_endpoint_t endpoint, net_endpoint_size_info_t size_info);
int net_ssl_stream_endpoint_connect(net_endpoint_t endpoint);
int net_ssl_stream_endpoint_update(net_endpoint_t endpoint);
int net_ssl_stream_endpoint_set_no_delay(net_endpoint_t endpoint, uint8_t no_delay);
int net_ssl_stream_endpoint_get_mss(net_endpoint_t endpoint, uint32_t * mss);

#endif
