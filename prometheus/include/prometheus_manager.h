#ifndef PROMETHEUS_MANAGER_H_INCLEDED
#define PROMETHEUS_MANAGER_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "prometheus_types.h"

NET_BEGIN_DECL

prometheus_manager_t
prometheus_manager_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule, net_driver_t driver);
void prometheus_manager_free(prometheus_manager_t manager);

NET_END_DECL

#endif
