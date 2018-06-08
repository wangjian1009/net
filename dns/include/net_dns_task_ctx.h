#ifndef NET_DNS_TASK_CTX_H_INCLEDED
#define NET_DNS_TASK_CTX_H_INCLEDED
#include "net_dns_system.h"

NET_BEGIN_DECL

net_dns_task_ctx_t net_dns_task_ctx_create(net_dns_task_step_t step, net_dns_source_t source);
void net_dns_task_ctx_free(net_dns_task_ctx_t ctx);

net_dns_task_t net_dns_task_ctx_task(net_dns_task_ctx_t ctx);
net_dns_task_step_t net_dns_task_ctx_step(net_dns_task_ctx_t ctx);
net_dns_source_t net_dns_task_ctx_source(net_dns_task_ctx_t ctx);

void * net_dns_task_ctx_data(net_dns_task_ctx_t ctx);

net_dns_task_state_t net_dns_task_ctx_state(net_dns_task_ctx_t ctx);

void net_dns_task_ctx_set_result(net_dns_task_ctx_t ctx, net_address_t address, uint8_t is_own);
void net_dns_task_ctx_set_error(net_dns_task_ctx_t ctx);

NET_END_DECL

#endif
