#ifndef NET_EBB_PROCESSOR_H
#define NET_EBB_PROCESSOR_H
#include "net_ebb_system.h"

NET_BEGIN_DECL

typedef void (*net_ebb_processor_env_clear_fun_t)(void * ctx, void * env);

net_ebb_processor_t
net_ebb_processor_create(
    net_ebb_service_t service, const char * name, void * ctx,
    /*env*/
    net_ebb_processor_env_clear_fun_t env_clear);

void net_ebb_processor_free(net_ebb_processor_t processor);

net_ebb_processor_t net_ebb_processor_find_by_name(net_ebb_service_t service, const char * name);

void * net_ebb_processor_ctx(net_ebb_processor_t processor);

NET_END_DECL

#endif
