#ifndef NET_DNS_TASK_BUILDER_H_INCLEDED
#define NET_DNS_TASK_BUILDER_H_INCLEDED
#include "net_dns_system.h"

NET_BEGIN_DECL

typedef int (*net_dns_task_builder_init_fun_t)(net_dns_task_builder_t builder);
typedef void (*net_dns_task_builder_fini_fun_t)(net_dns_task_builder_t builder);
typedef int (*net_dns_task_build_fun_t)(net_dns_task_builder_t builder, net_dns_task_t task, const char * policy);

net_dns_task_builder_t
net_dns_task_builder_create(
    net_dns_manage_t manage,
    uint32_t capacity,
    net_dns_task_builder_init_fun_t init,
    net_dns_task_builder_fini_fun_t fini,
    net_dns_task_build_fun_t build);

void net_dns_task_builder_free(net_dns_task_builder_t builder);

void * net_dns_task_builder_data(net_dns_task_builder_t builder);
net_dns_task_builder_t net_dns_task_builder_from_data(void * data);

int net_dns_task_builder_build(net_dns_task_builder_t builder, net_dns_task_t task, const char * policy);

NET_END_DECL

#endif
