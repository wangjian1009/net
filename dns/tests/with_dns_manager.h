#ifndef NET_WITH_DNS_MANAGE_H_INCLEDED
#define NET_WITH_DNS_MANAGE_H_INCLEDED
#include "cpe/pal/pal_types.h"
#include "check.h"
#include "cpe/utils/error.h"
#include "cpe/utils/memory.h"
#include "cpe/utils/buffer.h"
#include "../src/net_dns_manage_i.h"

typedef struct with_dns_manager * with_dns_manager_t;

struct with_dns_manager {
    mem_allocrator_t m_alloc;
    struct error_monitor m_em_buf;
    net_schedule_t m_schedule;
    net_dns_manage_t m_dns;
    struct mem_buffer m_tmp_buffer;
};
with_dns_manager_t with_dns_manager;

void with_dns_manager_setup(void);
void with_dns_manager_teardown(void);

void with_dns_manager_add_records(const char * records);

net_dns_entry_t with_dns_manager_find_entry(const char * adress);
const char * with_dns_manager_hostnames_by_ip(const char * str_ip);
const char * with_dns_manager_hostname_by_ip(const char * str_ip);
const char * with_dns_manager_entry_select_item(const char * entry, net_dns_query_type_t query_type);

#endif
