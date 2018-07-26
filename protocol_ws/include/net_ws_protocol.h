#ifndef NET_WS_PROTOCOL_H_INCLEDED
#define NET_WS_PROTOCOL_H_INCLEDED
#include "net_ws_types.h"

NET_BEGIN_DECL

typedef int (*net_ws_protocol_init_fun_t)(net_ws_protocol_t ws_protocol);
typedef void (*net_ws_protocol_fini_fun_t)(net_ws_protocol_t ws_protocol);

typedef int (*net_ws_endpoint_init_fun_t)(net_ws_endpoint_t ws_ep);
typedef void (*net_ws_endpoint_fini_fun_t)(net_ws_endpoint_t ws_ep);

net_ws_protocol_t net_ws_protocol_create(
    net_schedule_t schedule,
    const char * protocol_name,
    /*protocol*/
    uint16_t protocol_capacity,
    net_ws_protocol_init_fun_t protocol_init,
    net_ws_protocol_fini_fun_t protocol_fini,
    /*endpoint*/
    uint16_t endpoint_capacity,
    net_ws_endpoint_init_fun_t endpoint_init,
    net_ws_endpoint_fini_fun_t endpoint_fini);

void net_ws_protocol_free(net_ws_protocol_t ws_protocol);

NET_END_DECL

#endif
