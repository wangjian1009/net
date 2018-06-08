#ifndef NET_DNS_SOURCE_H_INCLEDED
#define NET_DNS_SOURCE_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_dns_system.h"

NET_BEGIN_DECL

typedef int (*net_dns_source_init_fun_t)(net_dns_source_t source);
typedef void (*net_dns_source_fini_fun_t)(net_dns_source_t source);
typedef void (*net_dns_source_dump_fun_t)(write_stream_t ws, net_dns_source_t source);

typedef int (*net_dns_task_ctx_init_fun_t)(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
typedef void (*net_dns_task_ctx_fini_fun_t)(net_dns_source_t source, net_dns_task_ctx_t task_ctx);

net_dns_source_t
net_dns_source_create(
    net_dns_manage_t manage,
    uint32_t capacity,
    net_dns_source_init_fun_t init,
    net_dns_source_fini_fun_t fini,
    net_dns_source_dump_fun_t dump,
    uint32_t task_ctx_capacity,
    net_dns_task_ctx_init_fun_t task_ctx_init,
    net_dns_task_ctx_fini_fun_t task_ctx_fini);

void net_dns_source_free(net_dns_source_t source);

void * net_dns_source_data(net_dns_source_t source);
net_dns_source_t net_dns_source_from_data(void * date);

NET_END_DECL

#endif
