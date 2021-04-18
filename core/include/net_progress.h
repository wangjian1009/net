#ifndef NET_PROGRESS_H_INCLEDED
#define NET_PROGRESS_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

enum net_progress_runing_mode {
    net_progress_runing_mode_read,
    net_progress_runing_mode_write,
};

enum net_progress_state {
    net_progress_state_runing,
    net_progress_state_complete,
    net_progress_state_error,
};

typedef void (*net_progress_update_fun_t)(void * ctx, net_progress_t progress);

enum net_progress_debug_mode {
    net_progress_debug_none,
    net_progress_debug_text,
    net_progress_debug_binary,
};

net_progress_t
net_progress_auto_create(
    net_schedule_t schedule,
    const char * cmd, const char * argv[],
    net_progress_runing_mode_t mode,
    net_progress_update_fun_t data_recv_fun, void * ctx,
    enum net_progress_debug_mode debug);

net_progress_t
net_progress_create(
    net_driver_t driver,
    const char * cmd, const char * argv[], 
    net_progress_runing_mode_t mode,
    net_progress_update_fun_t data_watch_fun, void * ctx,
    enum net_progress_debug_mode debug);

void net_progress_free(net_progress_t progress);

net_progress_t
net_progress_find(net_schedule_t schedule, uint32_t ep_id);

uint32_t net_progress_id(net_progress_t progress);
net_driver_t net_progress_driver(net_progress_t progress);
net_progress_runing_mode_t net_progress_runing_mode(net_progress_t progress);
net_progress_state_t net_progress_state(net_progress_t progress);

int net_progress_exit_state(net_progress_t progress);
int net_progress_errno(net_progress_t progress);
const char * net_progress_error_msg(net_progress_t progress);

int net_progress_set_complete(net_progress_t progress, int exit_stat);
int net_progress_set_error(net_progress_t progress, int err, const char * msg);

/*buf.write*/
void * net_progress_buf_alloc_at_least(net_progress_t progress, uint32_t * inout_size);
void * net_progress_buf_alloc_suggest(net_progress_t progress, uint32_t * inout_size);
void * net_progress_buf_alloc(net_progress_t progress, uint32_t size);
void net_progress_buf_release(net_progress_t progress);
int net_progress_buf_supply(net_progress_t progress, uint32_t size);
int net_progress_buf_append(net_progress_t progress, void const * data, uint32_t size);
int net_progress_buf_append_char(net_progress_t progress, uint8_t c);

/*buf.read*/
uint32_t net_progress_buf_size(net_progress_t progress);
void * net_progress_buf_peak(net_progress_t progress, uint32_t * size);
int net_progress_buf_peak_with_size(net_progress_t progress, uint32_t require, void * * data);
int net_progress_buf_recv(net_progress_t progress, void * data, uint32_t * size);
int net_progress_buf_by_sep(net_progress_t progress, const char * seps, void * * r_data, uint32_t * r_size);
int net_progress_buf_by_str(net_progress_t progress, const char * str, void * * r_data, uint32_t * r_size);
void net_progress_buf_consume(net_progress_t progress, uint32_t size);

/*dump*/
void net_progress_print(write_stream_t ws, net_progress_t progress);
const char * net_progress_dump(mem_buffer_t buff, net_progress_t progress);

/*utils*/
void * net_progress_data(net_progress_t progress);
net_progress_t net_progress_from_data(void * data);

const char * net_progress_runing_mode_str(net_progress_runing_mode_t mode);
const char * net_progress_state_str(net_progress_state_t state);

NET_END_DECL

#endif
