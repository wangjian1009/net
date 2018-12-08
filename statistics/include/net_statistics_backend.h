#ifndef NET_STATISTICS_BACKEND_H_INCLEDED
#define NET_STATISTICS_BACKEND_H_INCLEDED
#include "net_statistics_system.h"

CPE_BEGIN_DECL

typedef int (*net_statistics_backend_init_fun_t)(net_statistics_backend_t backend);
typedef void (*net_statistics_backend_fini_fun_t)(net_statistics_backend_t backend);

typedef void (*net_statistics_log_event_fun_t)(net_statistics_backend_t backend, const char *type, const char *name, const char *status, const char *data);
typedef void (*net_statistics_log_metric_for_count_fun_t)(net_statistics_backend_t backend, const char *name, int quantity);
typedef void (*net_statistics_log_metric_for_duration_fun_t)(net_statistics_backend_t backend, const char *name, uint64_t duration_ms);

net_statistics_backend_t
net_statistics_backend_create(
    net_statistics_t statistics,
    const char * name,
    uint16_t backend_capacity,
    net_statistics_backend_init_fun_t backend_init,
    net_statistics_backend_fini_fun_t backend_fini,
    net_statistics_log_event_fun_t log_event,
    net_statistics_log_metric_for_count_fun_t log_metric_for_count,
    net_statistics_log_metric_for_duration_fun_t log_metric_for_duration);

void net_statistics_backend_free(net_statistics_backend_t backend);

const char * net_statistics_backend_name(net_statistics_backend_t backend);

void * net_statistics_backend_data(net_statistics_backend_t backend);
net_statistics_backend_t net_statistics_backend_from_data(void * data);

CPE_END_DECL

#endif
