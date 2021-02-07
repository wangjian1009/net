#ifndef NET_DNS_TESTENV_H_INCLEDED
#define NET_DNS_TESTENV_H_INCLEDED
#include "cpe/pal/pal_types.h"
#include "test_memory.h"
#include "test_error.h"
#include "cpe/utils/buffer.h"
#include "../src/net_dns_manage_i.h"

typedef struct net_dns_testenv * net_dns_testenv_t;

struct net_dns_testenv {
    mem_allocrator_t m_alloc;
    struct error_monitor m_em_buf;
    net_schedule_t m_schedule;
    net_dns_manage_t m_dns;
    struct mem_buffer m_tmp_buffer;
};

net_dns_testenv_t net_dns_testenv_create();
void net_dns_testenv_free(net_dns_testenv_t env);

void net_dns_testenv_setup_records(net_dns_testenv_t env, const char * records);

int net_dns_testenv_add_record(net_dns_testenv_t env, const char * host, const char * resolve_to);
net_dns_entry_t net_dns_testenv_find_entry(net_dns_testenv_t env, const char * adress);
const char * net_dns_testenv_hostnames_by_ip(net_dns_testenv_t env, const char * str_ip);
const char * net_dns_testenv_hostname_by_ip(net_dns_testenv_t env, const char * str_ip);
const char * net_dns_testenv_entry_select_item(net_dns_testenv_t env, const char * entry, net_dns_query_type_t query_type);

#endif
