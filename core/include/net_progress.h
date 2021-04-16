#ifndef NET_PROGRESS_H_INCLEDED
#define NET_PROGRESS_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_system.h"

NET_BEGIN_DECL

enum net_progress_runing_mode {
    net_progress_runing_mode_read,
    net_progress_runing_mode_write,
};

typedef void (*net_progress_data_watch_fun_t)(void * ctx, net_progress_t progress);

net_progress_t
net_progress_auto_create(
    net_schedule_t schedule, const char * cmd, net_progress_runing_mode_t mode,
    net_progress_data_watch_fun_t data_recv_fun, void * ctx);

net_progress_t
net_progress_create(
    net_driver_t driver, const char * cmd, net_progress_runing_mode_t mode,
    net_progress_data_watch_fun_t data_watch_fun, void * ctx);

void net_progress_free(net_progress_t progress);

uint32_t net_progress_id(net_progress_t progress);
net_driver_t net_progress_driver(net_progress_t progress);
net_progress_runing_mode_t net_progress_runing_mode(net_progress_t progress);

/*utils*/
void * net_progress_data(net_progress_t progress);
net_progress_t net_progress_from_data(void * data);

const char * net_progress_runing_mode_str(net_progress_runing_mode_t mode);

NET_END_DECL

#endif
