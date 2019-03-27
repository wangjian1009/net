#ifndef NET_LOG_CATEGORY_H_INCLEDED
#define NET_LOG_CATEGORY_H_INCLEDED
#include "net_log_types.h"

NET_BEGIN_DECL

net_log_category_t net_log_category_create(net_log_schedule_t schedule, const char * name, uint8_t id);
void net_log_category_free(net_log_category_t category);

NET_END_DECL

#endif
