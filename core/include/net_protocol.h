#ifndef NET_PROTOCOL_H_INCLEDED
#define NET_PROTOCOL_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

typedef int (*net_protocol_init_fun_t)(net_protocol_t protocol);
typedef void (*net_protocol_fini_fun_t)(net_protocol_t protocol);

typedef int (*net_protocol_endpoint_init_fun_t)(net_endpoint_t endpoint);
typedef void (*net_protocol_endpoint_fini_fun_t)(net_endpoint_t endpoint);
typedef int (*net_protocol_endpoint_on_state_change_fun_t)(net_endpoint_t endpoint, net_endpoint_state_t from_state);
typedef int (*net_protocol_endpoint_input_fun_t)(net_endpoint_t endpoint);
typedef void (*net_protocol_endpoint_dump_fun_t)(write_stream_t ws, net_endpoint_t endpoint);

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
    net_protocol_endpoint_on_state_change_fun_t endpoint_on_state_chagne,
    net_protocol_endpoint_dump_fun_t endpoint_dump);

void net_protocol_free(net_protocol_t protocol);

net_protocol_t net_protocol_find(net_schedule_t schedule, const char * name);

net_schedule_t net_protocol_schedule(net_protocol_t protocol);
const char * net_protocol_name(net_protocol_t protocol);

void * net_protocol_data(net_protocol_t protocol);
net_protocol_t net_protocol_from_data(void * data);

uint8_t net_protocol_debug(net_protocol_t protocol);
void net_protocol_set_debug(net_protocol_t protocol, uint8_t debug);

net_protocol_init_fun_t net_protocol_init_fun(net_protocol_t protocol);

void net_protocol_endpoints(net_protocol_t protocol, net_endpoint_it_t endpoint_it);

NET_END_DECL

#endif
