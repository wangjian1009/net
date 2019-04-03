#ifndef NET_LOG_CATEGORY_H_INCLEDED
#define NET_LOG_CATEGORY_H_INCLEDED
#include "net_log_types.h"

NET_BEGIN_DECL

net_log_category_t net_log_category_create(
    net_log_schedule_t schedule, net_log_flusher_t flusher, net_log_sender_t sender, const char * name, uint8_t id);
void net_log_category_free(net_log_category_t category);

void net_log_category_set_compress(net_log_category_t category, net_log_compress_type_t compress);
int net_log_category_set_topic(net_log_category_t category, const char * topic);
void net_log_category_set_bytes_per_package(net_log_category_t category, uint32_t sz);
void net_log_category_set_count_per_package(net_log_category_t category, uint32_t count);
int net_log_category_add_global_tag(net_log_category_t category, const char * key, const char * value);

NET_END_DECL

#endif
