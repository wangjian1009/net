#ifndef NET_NGHTTP2_PROCESSOR_H
#define NET_NGHTTP2_PROCESSOR_H
#include "net_nghttp2_system.h"

NET_BEGIN_DECL

typedef int (*net_nghttp2_processor_request_init_fun_t)(void * ctx, void * env, net_nghttp2_request_t request);
typedef void (*net_nghttp2_processor_request_fini_fun_t)(void * ctx, net_nghttp2_request_t request);
typedef void (*net_nghttp2_processor_request_on_head_complete_fun_t)(void * ctx, net_nghttp2_request_t request);
typedef void (*net_nghttp2_processor_request_on_complete_fun_t)(void * ctx, net_nghttp2_request_t request);
typedef void (*net_nghttp2_processor_env_clear_fun_t)(void * ctx, void * env);

net_nghttp2_processor_t
net_nghttp2_processor_create(
    net_nghttp2_service_t service, const char * name, void * ctx,
    /*env*/
    net_nghttp2_processor_env_clear_fun_t env_clear,
    /*request*/
    uint16_t request_sz,
    net_nghttp2_processor_request_init_fun_t request_init,
    net_nghttp2_processor_request_fini_fun_t request_fini,
    net_nghttp2_processor_request_on_head_complete_fun_t requst_on_head_complete,
    net_nghttp2_processor_request_on_complete_fun_t requst_on_complete);

void net_nghttp2_processor_free(net_nghttp2_processor_t processor);

net_nghttp2_processor_t net_nghttp2_processor_find_by_name(net_nghttp2_service_t service, const char * name);

void * net_nghttp2_processor_ctx(net_nghttp2_processor_t processor);

NET_END_DECL

#endif
