#ifndef NET_HTTP2_CONTROL_ENDPOINT_I_H_INCLEDED
#define NET_HTTP2_CONTROL_ENDPOINT_I_H_INCLEDED
#include "cpe/utils/sha1.h"
#include "net_http2_control_endpoint.h"
#include "net_http2_control_protocol_i.h"

struct net_http2_control_endpoint {
    net_endpoint_t m_base_endpoint;
};

/**/
int net_http2_control_endpoint_init(net_endpoint_t base_endpoint);
void net_http2_control_endpoint_fini(net_endpoint_t base_endpoint);
int net_http2_control_endpoint_input(net_endpoint_t base_endpoint);
int net_http2_control_endpoint_on_state_change(net_endpoint_t base_endpoint, net_endpoint_state_t from_state);

#endif
