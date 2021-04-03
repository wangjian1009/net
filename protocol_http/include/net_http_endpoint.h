#ifndef NET_HTTP_ENDPOINT_H_INCLEDED
#define NET_HTTP_ENDPOINT_H_INCLEDED
#include "net_http_types.h"

NET_BEGIN_DECL

net_http_endpoint_t net_http_endpoint_create(
    net_driver_t driver, net_http_protocol_t http_protocol);
void net_http_endpoint_free(net_http_endpoint_t http_ep);

net_http_endpoint_t net_http_endpoint_get(net_endpoint_t endpoint);

net_http_protocol_t net_http_endpoint_protocol(net_http_endpoint_t http_ep);
net_endpoint_t net_http_endpoint_base_endpoint(net_http_endpoint_t http_ep);
net_schedule_t net_http_endpoint_schedule(net_http_endpoint_t http_ep);

net_endpoint_t net_http_endpoint_base_endpoint(net_http_endpoint_t http_ep);

uint32_t net_http_endpoint_reconnect_span_ms(net_http_endpoint_t http_ep);
void net_http_endpoint_set_reconnect_span_ms(net_http_endpoint_t http_ep, uint32_t span_ms);

net_http_connection_type_t net_http_endpoint_connection_type(net_http_endpoint_t http_ep);

const char * net_http_endpoint_request_id_tag(net_http_endpoint_t http_ep);
int net_http_endpoint_set_request_id_tag(net_http_endpoint_t http_ep, const char * tag);

int net_http_endpoint_flush(net_http_endpoint_t http_ep);

void * net_http_endpoint_data(net_http_endpoint_t http_ep);
net_http_endpoint_t net_http_endpoint_from_data(void * data);

NET_END_DECL

#endif
