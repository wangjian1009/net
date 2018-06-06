#ifndef NET_PROTOCOL_H_INCLEDED
#define NET_PROTOCOL_H_INCLEDED
#include "net_system.h"

NET_BEGIN_DECL

typedef int (*net_protocol_init_fun_t)(net_protocol_t protocol);
typedef void (*net_protocol_fini_fun_t)(net_protocol_t protocol);

typedef int (*net_protocol_endpoint_init_fun_t)(net_endpoint_t endpoint);
typedef void (*net_protocol_endpoint_fini_fun_t)(net_endpoint_t endpoint);
typedef int (*net_protocol_endpoint_input_fun_t)(net_endpoint_t endpoint);
typedef int (*net_protocol_endpoint_forward_fun_t)(net_endpoint_t endpoint, net_endpoint_t from);
typedef int (*net_protocol_endpoint_direct_fun_t)(net_endpoint_t endpoint, net_address_t target);

net_protocol_t
net_protocol_create(
    net_schedule_t schedule,
    const char * name,
    /*protocol*/
    uint16_t protocol_capacity,
    net_protocol_init_fun_t protocol_init,
    net_protocol_fini_fun_t protocol_fini,
    /*endpoint*/
    uint16_t endpoint_capacity,
    net_protocol_endpoint_init_fun_t endpoint_init,
    net_protocol_endpoint_fini_fun_t endpoint_fini,
    net_protocol_endpoint_input_fun_t endpoint_input,
    net_protocol_endpoint_forward_fun_t endpoint_forward,
    net_protocol_endpoint_direct_fun_t endpoint_direct);

void net_protocol_free(net_protocol_t protocol);

net_schedule_t net_protocol_schedule(net_protocol_t protocol);

void * net_protocol_data(net_protocol_t protocol);
net_protocol_t net_protocol_from_data(void * data);

NET_END_DECL

#endif
