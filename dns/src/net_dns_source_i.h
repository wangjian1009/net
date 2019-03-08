#ifndef NET_DNS_SOURCE_I_H_INCLEDED
#define NET_DNS_SOURCE_I_H_INCLEDED
#include "net_dns_source.h"
#include "net_dns_manage_i.h"

NET_BEGIN_DECL

struct net_dns_source {
    net_dns_manage_t m_manage;
    TAILQ_ENTRY(net_dns_source) m_next;
    net_dns_scope_source_list_t m_scopes;

    uint32_t m_capacity;
    net_dns_source_init_fun_t m_init;
    net_dns_source_fini_fun_t m_fini;
    net_dns_source_dump_fun_t m_dump;

    uint32_t m_task_ctx_capacity;
    net_dns_task_ctx_init_fun_t m_task_ctx_init;
    net_dns_task_ctx_fini_fun_t m_task_ctx_fini;
    net_dns_task_ctx_start_fun_t m_task_ctx_start;
    net_dns_task_ctx_cancel_fun_t m_task_ctx_cancel;

    net_dns_task_ctx_list_t m_ctxs;
};

NET_END_DECL

#endif
