#ifndef NET_HTTP_ENDPOINT_H_INCLEDED
#define NET_HTTP_ENDPOINT_H_INCLEDED
#include "net_http_types.h"

NET_BEGIN_DECL

#define net_ep_buf_http_in net_ep_buf_user1
#define net_ep_buf_http_out net_ep_buf_user2
#define net_ep_buf_http_body net_ep_buf_user3

net_http_endpoint_t net_http_endpoint_create(
    net_driver_t driver, net_endpoint_type_t type, net_http_protocol_t http_protocol);
void net_http_endpoint_free(net_http_endpoint_t http_ep);

net_http_endpoint_t net_http_endpoint_get(net_endpoint_t endpoint);

net_http_protocol_t net_http_endpoint_protocol(net_http_endpoint_t http_ep);
net_endpoint_t net_http_endpoint_net_ep(net_http_endpoint_t http_ep);
net_schedule_t net_http_endpoint_schedule(net_http_endpoint_t http_ep);

net_http_state_t net_http_endpoint_state(net_http_endpoint_t http_ep);
net_endpoint_t net_http_endpoint_net_ep(net_http_endpoint_t http_ep);

uint32_t net_http_endpoint_reconnect_span_ms(net_http_endpoint_t http_ep);
void net_http_endpoint_set_reconnect_span_ms(net_http_endpoint_t http_ep, uint32_t span_ms);

uint8_t net_http_endpoint_use_https(net_http_endpoint_t http_ep);

net_http_ssl_ctx_t net_http_endpoint_ssl_ctx(net_http_endpoint_t http_ep);
net_http_ssl_ctx_t net_http_endpoint_ssl_enable(net_http_endpoint_t http_ep);
int net_http_endpoint_ssl_disable(net_http_endpoint_t http_ep);

net_http_connection_type_t net_http_endpoint_connection_type(net_http_endpoint_t http_ep);

int net_http_endpoint_set_remote(net_http_endpoint_t http_ep, const char * url);

void net_http_endpoint_enable(net_http_endpoint_t http_ep);

int net_http_endpoint_flush(net_http_endpoint_t http_ep);

void * net_http_endpoint_data(net_http_endpoint_t http_ep);
net_http_endpoint_t net_http_endpoint_from_data(void * data);

const char * net_http_state_str(net_http_state_t state);

NET_END_DECL

#endif
