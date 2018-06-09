#ifndef NET_DNS_SOURCE_H_INCLEDED
#define NET_DNS_SOURCE_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_dns_system.h"

NET_BEGIN_DECL

struct net_dns_source_it {
    net_dns_source_t (*next)(struct net_dns_source_it * it);
    char m_data[64];
};

typedef int (*net_dns_source_init_fun_t)(net_dns_source_t source);
typedef void (*net_dns_source_fini_fun_t)(net_dns_source_t source);
typedef void (*net_dns_source_dump_fun_t)(write_stream_t ws, net_dns_source_t source);

typedef int (*net_dns_task_ctx_init_fun_t)(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
typedef void (*net_dns_task_ctx_fini_fun_t)(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
typedef int (*net_dns_task_ctx_start_fun_t)(net_dns_source_t source, net_dns_task_ctx_t task_ctx);
typedef void (*net_dns_task_ctx_cancel_fun_t)(net_dns_source_t source, net_dns_task_ctx_t task_ctx);

net_dns_source_t
net_dns_source_create(
    net_dns_manage_t manage,
    uint32_t capacity,
    net_dns_source_init_fun_t init,
    net_dns_source_fini_fun_t fini,
    net_dns_source_dump_fun_t dump,
    uint32_t task_ctx_capacity,
    net_dns_task_ctx_init_fun_t task_ctx_init,
    net_dns_task_ctx_fini_fun_t task_ctx_fini,
    net_dns_task_ctx_start_fun_t task_ctx_start,
    net_dns_task_ctx_cancel_fun_t task_ctx_cancel);

void net_dns_source_free(net_dns_source_t source);

void * net_dns_source_data(net_dns_source_t source);
net_dns_source_t net_dns_source_from_data(void * date);

void net_dns_source_print(write_stream_t ws, net_dns_source_t source);
const char * net_dns_source_dump(mem_buffer_t buffer, net_dns_source_t source);

void net_dns_manage_sources(net_dns_manage_t manage, net_dns_source_it_t it);

#define net_dns_source_it_next(it) ((it)->next ? (it)->next(it) : NULL)

NET_END_DECL

#endif
