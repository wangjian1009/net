#ifndef NET_DNS_TASK_STEP_I_H_INCLEDED
#define NET_DNS_TASK_STEP_I_H_INCLEDED
#include "net_dns_task_step.h"
#include "net_dns_task_i.h"

NET_BEGIN_DECL

struct net_dns_task_step {
    net_dns_task_t m_task;
    TAILQ_ENTRY(net_dns_task_step) m_next;
    net_dns_task_ctx_list_t m_ctxs;
};

void net_dns_task_step_real_free(net_dns_task_step_t task_step);

void net_dns_task_step_start(net_dns_task_step_t task_step);

NET_END_DECL

#endif
