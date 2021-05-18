#ifndef NET_ENDPOINT_H_INCLEDED
#define NET_ENDPOINT_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

struct net_endpoint_it {
    net_endpoint_t (*next)(net_endpoint_it_t it);
    char data[64];
};

struct net_endpoint_size_info {
    uint64_t m_read;
    uint64_t m_write;
};

net_endpoint_t net_endpoint_create(net_driver_t driver, net_protocol_t protocol, net_mem_group_t mem_group);
void net_endpoint_free(net_endpoint_t endpoint);

net_schedule_t net_endpoint_schedule(net_endpoint_t endpoint);
net_protocol_t net_endpoint_protocol(net_endpoint_t endpoint);
net_mem_group_t net_endpoint_mem_group(net_endpoint_t endpoint);
const char * net_endpoint_protocol_name(net_endpoint_t endpoint);
int net_endpoint_set_protocol(net_endpoint_t endpoint, net_protocol_t protocol);
net_driver_t net_endpoint_driver(net_endpoint_t endpoint);
const char *net_endpoint_driver_name(net_endpoint_t endpoint);

uint32_t net_endpoint_id(net_endpoint_t endpoint);
net_endpoint_t net_endpoint_find(net_schedule_t schedule, uint32_t id);

net_address_t net_endpoint_address(net_endpoint_t endpoint);
int net_endpoint_set_address(net_endpoint_t endpoint, net_address_t address);

uint32_t net_endpoint_suggest_block_size(net_endpoint_t endpoint);
void net_endpoint_calc_size(net_endpoint_t endpoint, net_endpoint_size_info_t size_info);

uint8_t net_endpoint_expect_read(net_endpoint_t endpoint);
void net_endpoint_set_expect_read(net_endpoint_t endpoint, uint8_t expect_read);

uint8_t net_endpoint_is_writing(net_endpoint_t endpoint);
void net_endpoint_set_is_writing(net_endpoint_t endpoint, uint8_t is_writing);

net_address_t net_endpoint_remote_address(net_endpoint_t endpoint);
int net_endpoint_set_remote_address(net_endpoint_t endpoint, net_address_t address);

net_endpoint_state_t net_endpoint_state(net_endpoint_t endpoint);
int net_endpoint_set_state(net_endpoint_t endpoint, net_endpoint_state_t state);

void * net_endpoint_data(net_endpoint_t endpoint);
net_endpoint_t net_endpoint_from_data(void * data);

int net_endpoint_set_prepare_connect(net_endpoint_t endpoint, net_endpoint_prepare_connect_fun_t fun, void * ctx);

uint8_t net_endpoint_is_active(net_endpoint_t endpoint);
uint8_t net_endpoint_is_readable(net_endpoint_t endpoint);
uint8_t net_endpoint_is_writeable(net_endpoint_t endpoint);
int net_endpoint_connect(net_endpoint_t endpoint);

uint8_t net_endpoint_close_after_send(net_endpoint_t endpoint);
void net_endpoint_set_close_after_send(net_endpoint_t endpoint);

uint8_t net_endpoint_protocol_debug(net_endpoint_t endpoint);
void net_endpoint_set_protocol_debug(net_endpoint_t endpoint, uint8_t debug);

uint8_t net_endpoint_driver_debug(net_endpoint_t endpoint);
void net_endpoint_set_driver_debug(net_endpoint_t endpoint, uint8_t debug);

uint32_t net_endpoint_all_buf_size(net_endpoint_t endpoint);

void net_endpoint_update_debug_info(net_endpoint_t endpoint);

int net_endpoint_set_no_delay(net_endpoint_t endpoint, uint8_t is_enable);
int net_endpoint_get_mss(net_endpoint_t endpoint, uint32_t * mss);

uint8_t net_endpoint_auto_free(net_endpoint_t http_ep);
int net_endpoint_set_auto_free(net_endpoint_t http_ep, uint8_t auto_close);

/*error info*/
void net_endpoint_set_error(net_endpoint_t endpoint, net_endpoint_error_source_t error_source, uint32_t error_no, const char * msg);
net_endpoint_error_source_t net_endpoint_error_source(net_endpoint_t endpoint);
uint32_t net_endpoint_error_no(net_endpoint_t endpoint);
const char * net_endpoint_error_msg(net_endpoint_t endpoint);
uint8_t net_endpoint_have_error(net_endpoint_t endpoint);

/*    check */
uint32_t net_endpoint_buf_size(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type);
uint32_t net_endpoint_buf_size_total(net_endpoint_t endpoint);
uint8_t net_endpoint_buf_is_empty(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type);
void net_endpoint_buf_clear(net_endpoint_t endpoint, net_endpoint_buf_type_t buf_type);

uint8_t net_endpoint_have_any_data(net_endpoint_t endpoint);

/*    connect*/
uint8_t net_endpoint_shift_address(net_endpoint_t endpoint);

/*    write*/
void * net_endpoint_buf_alloc_at_least(net_endpoint_t endpoint, uint32_t * inout_size);
void * net_endpoint_buf_alloc_suggest(net_endpoint_t endpoint, uint32_t * inout_size);
void * net_endpoint_buf_alloc(net_endpoint_t endpoint, uint32_t size);
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

/*dump*/
void net_endpoint_print(write_stream_t ws, net_endpoint_t endpoint);
const char * net_endpoint_dump(mem_buffer_t buff, net_endpoint_t endpoint);

/*data-watch*/
void net_endpoint_set_data_watcher(
    net_endpoint_t endpoint,
    void * watcher_ctx,
    net_endpoint_data_watch_fun_t watcher_fun,
    net_endpoint_data_watch_ctx_fini_fun_t watcher_ctx_fini);

/**/
void net_endpoint_clear_monitor_by_ctx(net_endpoint_t endpoint, void * ctx);

/*protocol*/
void * net_endpoint_protocol_data(net_endpoint_t endpoint);
net_endpoint_t net_endpoint_from_protocol_data(net_schedule_t schedule, void * data);

/**/
const char * net_endpoint_network_errno_str(net_endpoint_network_errno_t error);
const char * net_endpoint_error_source_str(net_endpoint_error_source_t source);
const char * net_endpoint_state_str(net_endpoint_state_t state);
const char * net_endpoint_buf_type_str(net_endpoint_buf_type_t buf_type);
const char * net_endpoint_data_event_str(net_endpoint_data_event_t data_evt);

/*net_endpoint_it*/
#define net_endpoint_it_next(__it) ((__it)->next(__it))

NET_END_DECL

#endif
