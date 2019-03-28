#ifndef NET_LOG_FLUSHER_H_INCLEDED
#define NET_LOG_FLUSHER_H_INCLEDED
#include "net_log_types.h"

NET_BEGIN_DECL

net_log_flusher_t net_log_flusher_create(net_log_schedule_t schedule, const char * name);
void net_log_flusher_free(net_log_flusher_t flusher);

net_log_flusher_t net_log_flusher_find(net_log_schedule_t schedule, const char * name);

NET_END_DECL

#endif
