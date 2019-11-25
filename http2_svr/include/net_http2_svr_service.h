#ifndef NET_HTTP2_SVR_SERVICE_H_INCLEDED
#define NET_HTTP2_SVR_SERVICE_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_http2_svr_system.h"

NET_BEGIN_DECL

net_http2_svr_service_t net_http2_svr_service_create(net_schedule_t schedule, const char * protocol_name);
void net_http2_svr_service_free(net_http2_svr_service_t service);

net_protocol_t net_http2_svr_service_to_protocol(net_http2_svr_service_t service);

const char * net_http2_svr_service_mine_from_postfix(net_http2_svr_service_t service, const char * postfix);

NET_END_DECL

#endif

