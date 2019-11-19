#ifndef NET_DQ_DRIVER_H_INCLEDED
#define NET_DQ_DRIVER_H_INCLEDED
#include <dispatch/dispatch.h>
#include "cpe/utils/utils_types.h"
#include "net_dq_types.h"

NET_BEGIN_DECL

net_dq_driver_t net_dq_driver_create(net_schedule_t schedule);

void net_dq_driver_free(net_dq_driver_t driver);

void net_dq_driver_set_data_monitor(
    net_dq_driver_t driver,
    net_data_monitor_fun_t monitor_fun, void * monitor_ctx);

net_driver_t net_dq_driver_base_driver(net_dq_driver_t driver);

NET_END_DECL

#endif
