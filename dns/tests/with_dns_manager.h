#ifndef NET_WITH_DNS_MANAGE_H_INCLEDED
#define NET_WITH_DNS_MANAGE_H_INCLEDED
#include "check.h"
#include "cpe/utils/error.h"
#include "cpe/utils/memory.h"
#include "net_dns_manage.h"

typedef struct with_dns_manager * with_dns_manager_t;

struct with_dns_manager {
    mem_allocrator_t m_alloc;
    struct error_monitor m_em_buf;
    net_schedule_t m_schedule;
    net_dns_manage_t m_dns;
};
with_dns_manager_t with_dns_manager;

void with_dns_manager_setup(void);
void with_dns_manager_teardown(void);

#endif
