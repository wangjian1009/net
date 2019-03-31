#ifndef NET_LOG_SENDER_H_INCLEDED
#define NET_LOG_SENDER_H_INCLEDED
#include "net_log_types.h"

NET_BEGIN_DECL

net_log_sender_t net_log_sender_create(net_log_schedule_t schedule, const char * name);
void net_log_sender_free(net_log_sender_t sender);

net_log_sender_t net_log_sender_find(net_log_schedule_t schedule, const char * name);

NET_END_DECL

#endif
