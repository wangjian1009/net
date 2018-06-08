#ifndef NET_DNS_TASK_CTX_I_H_INCLEDED
#define NET_DNS_TASK_CTX_I_H_INCLEDED
#include "net_dns_task_ctx.h"
#include "net_dns_task_step_i.h"
#include "net_dns_source_i.h"

NET_BEGIN_DECL

struct net_dns_task_ctx {
    net_dns_task_step_t m_step;
    TAILQ_ENTRY(net_dns_task_ctx) m_next_for_step;
    net_dns_source_t m_source;
    TAILQ_ENTRY(net_dns_task_ctx) m_next_for_source;
};

void net_dns_task_ctx_real_free(net_dns_task_ctx_t task_ctx);

NET_END_DECL

#endif
