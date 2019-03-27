#ifndef NET_LOG_SCHEDULE_H_INCLEDED
#define NET_LOG_SCHEDULE_H_INCLEDED
#include "net_log_types.h"

NET_BEGIN_DECL

net_log_schedule_t net_log_schedule_create(
    mem_allocrator_t alloc, error_monitor_t em, net_schedule_t schedule, net_driver_t driver);

void net_log_schedule_free(net_log_schedule_t log_schedule);

void net_log_begin(net_log_schedule_t schedule, uint8_t log_type);
void net_log_append_int32(net_log_schedule_t schedule, const char * name, int32_t value);
void net_log_append_uint32(net_log_schedule_t schedule, const char * name, uint32_t value);
void net_log_append_int64(net_log_schedule_t schedule, const char * name, int64_t value);
void net_log_append_uint64(net_log_schedule_t schedule, const char * name, uint64_t value);
void net_log_append_str(net_log_schedule_t schedule, const char * name, const char * value);
void net_log_append_md5(net_log_schedule_t schedule, const char * name, cpe_md5_value_t value);
void net_log_append_net_address(net_log_schedule_t schedule, const char * name, net_address_t address);
void net_log_commit(net_log_schedule_t schedule);

NET_END_DECL

#endif
