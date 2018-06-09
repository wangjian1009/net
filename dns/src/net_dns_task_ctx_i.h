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
    net_dns_task_state_t m_state;
    net_timer_t m_timeout_timer;
    uint16_t m_timeout_ms;
    uint16_t m_retry_count;
};

void net_dns_task_ctx_real_free(net_dns_task_ctx_t task_ctx);

void net_dns_task_ctx_start(net_dns_task_ctx_t task_ctx);

NET_END_DECL

#endif
