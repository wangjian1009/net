#ifndef NET_HTTP2_STREAM_BINDING_I_H_INCLEDED
#define NET_HTTP2_STREAM_BINDING_I_H_INCLEDED
#include "net_http2_stream_driver_i.h"

struct net_http2_stream_using {
    net_http2_stream_driver_t m_driver;
    net_http2_stream_using_list_t * m_owner;
    TAILQ_ENTRY(net_http2_stream_using) m_next;
    net_http2_endpoint_t m_http2_ep;
    net_endpoint_monitor_t m_monitor;
    net_http2_stream_endpoint_list_t m_streams;
};

net_http2_stream_using_t
net_http2_stream_using_create(
    net_http2_stream_driver_t driver, net_http2_stream_using_list_t * owner, net_http2_endpoint_t http2_ep);

void net_http2_stream_using_free(net_http2_stream_using_t using);
                                                                
#endif
