#ifndef NET_HTTP2_ENDPOINT_H_INCLEDED
#define NET_HTTP2_ENDPOINT_H_INCLEDED
#include "net_http2_types.h"

NET_BEGIN_DECL

enum net_http2_endpoint_runing_mode {
    net_http2_endpoint_runing_mode_init,
    net_http2_endpoint_runing_mode_cli,
    net_http2_endpoint_runing_mode_svr,
};

net_http2_endpoint_t net_http2_endpoint_cast(net_endpoint_t endpoint);

void net_http2_endpoint_free(net_http2_endpoint_t endpoint);

net_http2_endpoint_runing_mode_t net_http2_endpoint_runing_mode(net_http2_endpoint_t endpoint);
int net_http2_endpoint_set_runing_mode(net_http2_endpoint_t endpoint, net_http2_endpoint_runing_mode_t runing_mode);

net_endpoint_t net_http2_endpoint_base_endpoint(net_http2_endpoint_t endpoint);

const char * net_http2_endpoint_state_str(net_http2_endpoint_state_t state);
const char * net_http2_endpoint_runing_mode_str(net_http2_endpoint_runing_mode_t runing_mode);

NET_END_DECL

#endif
