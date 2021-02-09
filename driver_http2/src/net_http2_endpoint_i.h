#ifndef NET_HTTP2_STREAM_ENDPOINT_I_H_INCLEDED
#define NET_HTTP2_STREAM_ENDPOINT_I_H_INCLEDED
#include "net_http2_endpoint.h"
#include "net_http2_driver_i.h"

struct net_http2_endpoint {
    net_endpoint_t m_base_endpoint;
    net_http2_control_endpoint_t m_control;
};

int net_http2_endpoint_init(net_endpoint_t endpoint);
void net_http2_endpoint_fini(net_endpoint_t endpoint);
int net_http2_endpoint_connect(net_endpoint_t endpoint);
int net_http2_endpoint_update(net_endpoint_t endpoint);
int net_http2_endpoint_set_no_delay(net_endpoint_t endpoint, uint8_t no_delay);
int net_http2_endpoint_get_mss(net_endpoint_t endpoint, uint32_t * mss);

#endif
