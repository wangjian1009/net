#ifndef NET_DNS_TASK_BUILDER_I_H_INCLEDED
#define NET_DNS_TASK_BUILDER_I_H_INCLEDED
#include "net_dns_task_builder.h"
#include "net_dns_manage_i.h"

NET_BEGIN_DECL

struct net_dns_task_builder {
    net_dns_manage_t m_manage;
    TAILQ_ENTRY(net_dns_task_builder) m_next;
    
    uint32_t m_capacity;
    net_dns_task_builder_init_fun_t m_init;
    net_dns_task_builder_fini_fun_t m_fini;
    net_dns_task_build_fun_t m_build;
};

net_dns_task_builder_t net_dns_task_builder_internal_create(net_dns_manage_t manage);

NET_END_DECL

#endif
