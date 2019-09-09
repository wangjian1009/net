#ifndef NET_LOG_CATEGORY_H_INCLEDED
#define NET_LOG_CATEGORY_H_INCLEDED
#include "net_log_types.h"

NET_BEGIN_DECL

struct net_log_category_statistic {
    uint32_t m_loaded_record_count;
    uint32_t m_loaded_package_count;
    uint32_t m_record_count;
    uint32_t m_package_count;
    uint32_t m_package_success_count;
    uint32_t m_package_discard_count[net_log_discard_reason_count];
};

struct net_log_category_it {
    net_log_category_t (*next)(net_log_category_it_t it);
    char data[64];
};

net_log_category_t net_log_category_create(
    net_log_schedule_t schedule, net_log_thread_t flusher, net_log_thread_t sender, const char * name, uint8_t id);
void net_log_category_free(net_log_category_t category);

net_log_category_t net_log_category_find_by_id(net_log_schedule_t schedule, uint8_t id);
net_log_category_t net_log_category_find_by_name(net_log_schedule_t schedule, const char * name);

const char * net_log_category_name(net_log_category_t category);
int net_log_category_set_topic(net_log_category_t category, const char * topic);
void net_log_category_set_bytes_per_package(net_log_category_t category, uint32_t sz);
void net_log_category_set_count_per_package(net_log_category_t category, uint32_t count);
int net_log_category_add_global_tag(net_log_category_t category, const char * key, const char * value);

uint8_t net_log_category_enable(net_log_category_t category);
void net_log_category_set_enable(net_log_category_t category, uint8_t is_enable);

uint32_t net_log_category_timeout_ms(net_log_category_t category);
void net_log_category_set_timeout_ms(net_log_category_t category, uint32_t timeout_ms);

net_log_category_statistic_t net_log_category_statistic(net_log_category_t category);

void net_log_categories(net_log_schedule_t schedule, net_log_category_it_t category_it);

#define net_log_category_it_next(__it) ((__it)->next(__it))

NET_END_DECL

#endif
