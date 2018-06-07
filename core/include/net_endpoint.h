#ifndef NET_ENDPOINT_H_INCLEDED
#define NET_ENDPOINT_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

net_endpoint_t net_endpoint_create(
    net_driver_t driver, net_endpoint_type_t type, net_protocol_t protocol);
void net_endpoint_free(net_endpoint_t endpoint);

net_schedule_t net_endpoint_schedule(net_endpoint_t endpoint);
net_endpoint_type_t net_endpoint_type(net_endpoint_t endpoint);
net_protocol_t net_endpoint_protocol(net_endpoint_t endpoint);
net_driver_t net_endpoint_driver(net_endpoint_t endpoint);

uint32_t net_endpoint_id(net_endpoint_t endpoint);
net_endpoint_t net_endpoint_find(net_schedule_t schedule, uint32_t id);

net_address_t net_endpoint_address(net_endpoint_t endpoint);
int net_endpoint_set_address(net_endpoint_t endpoint, net_address_t address, uint8_t is_own);

net_address_t net_endpoint_remote_address(net_endpoint_t endpoint);
int net_endpoint_set_remote_address(net_endpoint_t endpoint, net_address_t address, uint8_t is_own);

net_endpoint_state_t net_endpoint_state(net_endpoint_t endpoint);
void net_endpoint_set_state(net_endpoint_t endpoint, net_endpoint_state_t state);

void * net_endpoint_data(net_endpoint_t endpoint);
net_endpoint_t net_endpoint_from_data(void * data);

int net_endpoint_connect(net_endpoint_t endpoint);
void net_endpoint_disable(net_endpoint_t endpoint);

/*link*/
int net_endpoint_link_with_router(net_endpoint_t endpoint, net_address_t target_addr, net_router_t router);
int net_endpoint_link_direct(net_endpoint_t endpoint, net_address_t target_addr);
int net_endpoint_link_auto_select(net_endpoint_t endpoint, net_address_t target_addr);
net_link_t net_endpoint_link(net_endpoint_t endpoint);
net_endpoint_t net_endpoint_other(net_endpoint_t endpoint);

/*rbuf*/
/*    --> */
uint8_t net_endpoint_rbuf_is_full(net_endpoint_t endpoint);
void * net_endpoint_rbuf_alloc(net_endpoint_t endpoint, uint32_t * inout_size);
int net_endpoint_rbuf_supply(net_endpoint_t endpoint, uint32_t size);

/*    <-- */
uint32_t net_endpoint_rbuf_size(net_endpoint_t endpoint);
void net_endpoint_rbuf_consume(net_endpoint_t endpoint, uint32_t size);
int net_endpoint_rbuf(net_endpoint_t endpoint, uint32_t require, void * * data);

/*wbuf*/
/*    <-- */
uint8_t net_endpoint_wbuf_is_empty(net_endpoint_t endpoint);
uint32_t net_endpoint_wbuf_size(net_endpoint_t endpoint);
void * net_endpoint_wbuf(net_endpoint_t endpoint, uint32_t * size);
void * net_endpoint_wbuf_alloc(net_endpoint_t endpoint, uint32_t * inout_size);
int net_endpoint_wbuf_supply(net_endpoint_t endpoint, uint32_t size);
void net_endpoint_wbuf_consume(net_endpoint_t endpoint, uint32_t size);

/*    --> */
int net_endpoint_wbuf_append(net_endpoint_t endpoint, void const * data, uint32_t size);
int net_endpoint_wbuf_append_from_other(net_endpoint_t endpoint, net_endpoint_t other, uint32_t size);

/*forward buf*/
/*    --> */
int net_endpoint_fbuf_append(net_endpoint_t endpoint, void const * data, uint32_t size);
int net_endpoint_fbuf_append_from_rbuf(net_endpoint_t endpoint, uint32_t size);

/*    <-- */
uint32_t net_endpoint_fbuf_size(net_endpoint_t endpoint);
int net_endpoint_fbuf(net_endpoint_t endpoint, uint32_t require, void * * data);
void net_endpoint_fbuf_consume(net_endpoint_t endpoint, uint32_t size);

/*forward*/
int net_endpoint_forward(net_endpoint_t endpoint);

/*dump*/
void net_endpoint_print(write_stream_t ws, net_endpoint_t endpoint);
const char * net_endpoint_dump(mem_buffer_t buff, net_endpoint_t endpoint);

/*protocol*/
void * net_endpoint_protocol_data(net_endpoint_t endpoint);

/**/
const char * net_endpoint_state_str(net_endpoint_state_t state);

NET_END_DECL

#endif
