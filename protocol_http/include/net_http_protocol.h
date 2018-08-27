#ifndef NET_HTTP_PROTOCOL_H_INCLEDED
#define NET_HTTP_PROTOCOL_H_INCLEDED
#include "net_http_types.h"

NET_BEGIN_DECL

typedef int (*net_http_protocol_init_fun_t)(net_http_protocol_t http_protocol);
typedef void (*net_http_protocol_fini_fun_t)(net_http_protocol_t http_protocol);

typedef int (*net_http_endpoint_init_fun_t)(net_http_endpoint_t http_ep);
typedef void (*net_http_endpoint_fini_fun_t)(net_http_endpoint_t http_ep);
typedef int (*net_http_endpoint_input_fun_t)(net_http_endpoint_t http_ep);
typedef int (*net_http_endpoint_on_state_change_fun_t)(net_http_endpoint_t http_ep, net_http_state_t from_state);

net_http_protocol_t
net_http_protocol_create(
    net_schedule_t schedule,
    const char * protocol_name,
    /*protocol*/
    uint16_t protocol_capacity,
    net_http_protocol_init_fun_t protocol_init,
    net_http_protocol_fini_fun_t protocol_fini,
    /*endpoint*/
    uint16_t endpoint_capacity,
    net_http_endpoint_init_fun_t endpoint_init,
    net_http_endpoint_fini_fun_t endpoint_fini,
    net_http_endpoint_input_fun_t endpoint_input_for_upgraded,
    net_http_endpoint_on_state_change_fun_t endpoint_on_state_change);

void net_http_protocol_free(net_http_protocol_t http_protocol);

net_schedule_t net_http_protocol_schedule(net_http_protocol_t http_protocol);

void * net_http_protocol_data(net_http_protocol_t http_protocol);
net_http_protocol_t net_http_protocol_from_data(void * data);

NET_END_DECL

#endif
