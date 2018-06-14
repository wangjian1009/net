#ifndef NET_DNS_TASK_CTX_H_INCLEDED
#define NET_DNS_TASK_CTX_H_INCLEDED
#include "net_dns_system.h"

NET_BEGIN_DECL

struct net_dns_task_ctx_it {
    net_dns_task_ctx_t (*next)(struct net_dns_task_ctx_it * it);
    char m_data[64];
};

net_dns_task_ctx_t net_dns_task_ctx_create(net_dns_task_step_t step, net_dns_source_t source);
void net_dns_task_ctx_free(net_dns_task_ctx_t ctx);

net_dns_manage_t net_dns_task_ctx_manage(net_dns_task_ctx_t ctx);
net_dns_task_t net_dns_task_ctx_task(net_dns_task_ctx_t ctx);
net_dns_task_step_t net_dns_task_ctx_step(net_dns_task_ctx_t ctx);
net_dns_source_t net_dns_task_ctx_source(net_dns_task_ctx_t ctx);

void * net_dns_task_ctx_data(net_dns_task_ctx_t ctx);

net_dns_task_state_t net_dns_task_ctx_state(net_dns_task_ctx_t ctx);

uint16_t net_dns_task_ctx_retry_count(net_dns_task_ctx_t ctx);
void net_dns_task_ctx_set_retry_count(net_dns_task_ctx_t ctx, uint16_t count);

uint16_t net_dns_task_ctx_timeout(net_dns_task_ctx_t ctx);
int net_dns_task_ctx_set_timeout(net_dns_task_ctx_t ctx, uint16_t timeout_ms);

void net_dns_task_ctx_set_success(net_dns_task_ctx_t ctx);
void net_dns_task_ctx_set_error(net_dns_task_ctx_t ctx);

#define net_dns_task_ctx_it_next(it) ((it)->next ? (it)->next(it) : NULL)

NET_END_DECL

#endif
