#ifndef NET_ENDPOINT_H_INCLEDED
#define NET_ENDPOINT_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

net_endpoint_t net_endpoint_create(net_driver_t driver, net_protocol_t protocol);
void net_endpoint_free(net_endpoint_t endpoint);

net_schedule_t net_endpoint_schedule(net_endpoint_t endpoint);
net_protocol_t net_endpoint_protocol(net_endpoint_t endpoint);
const char * net_endpoint_protocol_name(net_endpoint_t endpoint);
int net_endpoint_set_protocol(net_endpoint_t endpoint, net_protocol_t protocol);
net_driver_t net_endpoint_driver(net_endpoint_t endpoint);

uint32_t net_endpoint_id(net_endpoint_t endpoint);
net_endpoint_t net_endpoint_find(net_schedule_t schedule, uint32_t id);

net_address_t net_endpoint_address(net_endpoint_t endpoint);
int net_endpoint_set_address(net_endpoint_t endpoint, net_address_t address, uint8_t is_own);

net_address_t net_endpoint_remote_address(net_endpoint_t endpoint);
int net_endpoint_set_remote_address(net_endpoint_t endpoint, net_address_t address, uint8_t is_own);

net_endpoint_state_t net_endpoint_state(net_endpoint_t endpoint);
int net_endpoint_set_state(net_endpoint_t endpoint, net_endpoint_state_t state);

void * net_endpoint_data(net_endpoint_t endpoint);
net_endpoint_t net_endpoint_from_data(void * data);

int net_endpoint_set_prepare_connect(net_endpoint_t endpoint, net_endpoint_prepare_connect_fun_t fun, void * ctx);

uint8_t net_endpoint_is_active(net_endpoint_t endpoint);
int net_endpoint_connect(net_endpoint_t endpoint);
int net_endpoint_direct(net_endpoint_t endpoint, net_address_t target_addr);

uint8_t net_endpoint_close_after_send(net_endpoint_t endpoint);
void net_endpoint_set_close_after_send(net_endpoint_t endpoint, uint8_t is_close_after_send);

uint8_t net_endpoint_protocol_debug(net_endpoint_t endpoint);
void net_endpoint_set_protocol_debug(net_endpoint_t endpoint, uint8_t debug);

uint8_t net_endpoint_driver_debug(net_endpoint_t endpoint);
void net_endpoint_set_driver_debug(net_endpoint_t endpoint, uint8_t debug);

void net_endpoint_update_debug_info(net_endpoint_t endpoint);

/*error info*/
void net_endpoint_set_error(net_endpoint_t endpoint, net_endpoint_error_source_t error_source, uint32_t error_no);
net_endpoint_error_source_t net_endpoint_error_source(net_endpoint_t endpoint);
uint32_t net_endpoint_error_no(net_endpoint_t endpoint);
uint8_t net_endpoint_have_error(net_endpoint_t endpoint);

/*link*/
int net_endpoint_link_direct(net_endpoint_t endpoint, net_address_t target_addr, uint8_t is_own);
net_link_t net_endpoint_link(net_endpoint_t endpoint);
net_endpoint_t net_endpoint_other(net_endpoint_t endpoint);

/*buf*/
/*    check */
uint32_t net_endpoint_buf_size(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type);
uint8_t net_endpoint_buf_is_empty(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type);
uint8_t net_endpoint_buf_is_full(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type);
void net_endpoint_buf_clear(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type);

uint8_t net_endpoint_have_any_data(net_endpoint_t endpoint);

/*    write*/
void * net_endpoint_buf_alloc(net_endpoint_t endpoint, uint32_t * inout_size);
void net_endpoint_buf_release(net_endpoint_t endpoint);
int net_endpoint_buf_supply(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, uint32_t size);
int net_endpoint_buf_append(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, void const * data, uint32_t size);
int net_endpoint_buf_append_char(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, uint8_t c);

/*    read*/
void * net_endpoint_buf_peak(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, uint32_t * size);
int net_endpoint_buf_peak_with_size(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, uint32_t require, void * * data);
int net_endpoint_buf_recv(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, void * data, uint32_t * size);
int net_endpoint_buf_by_sep(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, const char * seps, void * * r_data, uint32_t * r_size);
int net_endpoint_buf_by_str(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, const char * str, void * * r_data, uint32_t * r_size);
void net_endpoint_buf_consume(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type, uint32_t size);

/*    move*/
int net_endpoint_buf_append_from_other(
    net_endpoint_t endpoint, net_endpoint_buf_type_t to,
    net_endpoint_t other, net_endpoint_buf_type_t from, uint32_t size);

int net_endpoint_buf_append_from_self(
    net_endpoint_t endpoint, net_endpoint_buf_type_t to, net_endpoint_buf_type_t from, uint32_t size);

/*forward*/
int net_endpoint_forward(net_endpoint_t endpoint);

/*dump*/
void net_endpoint_print(write_stream_t ws, net_endpoint_t endpoint);
const char * net_endpoint_dump(mem_buffer_t buff, net_endpoint_t endpoint);

/*data-watch*/
void net_endpoint_set_data_watcher(
    net_endpoint_t endpoint,
    void * watcher_ctx,
    net_endpoint_data_watch_fun_t watcher_fun,
    void(*watcher_ctx_fini)(void*, net_endpoint_t endpoint));

/**/
void net_endpoint_clear_monitor_by_ctx(net_endpoint_t endpoint, void * ctx);

/*protocol*/
void * net_endpoint_protocol_data(net_endpoint_t endpoint);

/**/
const char * net_endpoint_state_str(net_endpoint_state_t state);
const char * net_endpoint_buf_type_str(net_endpoint_buf_type_t buf_type);
const char * net_endpoint_data_event_str(net_endpoint_data_event_t data_evt);

NET_END_DECL

#endif
