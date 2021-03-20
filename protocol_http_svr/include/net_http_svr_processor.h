#ifndef NET_HTTP_SVR_PROCESSOR_H
#define NET_HTTP_SVR_PROCESSOR_H
#include "net_http_svr_system.h"

NET_BEGIN_DECL

typedef int (*net_http_svr_processor_request_init_fun_t)(void * ctx, void * env, net_http_svr_request_t request);
typedef void (*net_http_svr_processor_request_fini_fun_t)(void * ctx, net_http_svr_request_t request);
typedef void (*net_http_svr_processor_request_on_head_complete_fun_t)(void * ctx, net_http_svr_request_t request);
typedef void (*net_http_svr_processor_request_on_complete_fun_t)(void * ctx, net_http_svr_request_t request);
typedef void (*net_http_svr_processor_env_clear_fun_t)(void * ctx, void * env);

net_http_svr_processor_t
net_http_svr_processor_create(
    net_http_svr_protocol_t service, const char * name, void * ctx,
    /*env*/
    net_http_svr_processor_env_clear_fun_t env_clear,
    /*request*/
    uint16_t request_sz,
    net_http_svr_processor_request_init_fun_t request_init,
    net_http_svr_processor_request_fini_fun_t request_fini,
    net_http_svr_processor_request_on_head_complete_fun_t requst_on_head_complete,
    net_http_svr_processor_request_on_complete_fun_t requst_on_complete);

void net_http_svr_processor_free(net_http_svr_processor_t processor);

net_http_svr_processor_t net_http_svr_processor_find_by_name(net_http_svr_protocol_t service, const char * name);

void * net_http_svr_processor_ctx(net_http_svr_processor_t processor);

NET_END_DECL

#endif
