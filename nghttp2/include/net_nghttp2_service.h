#ifndef NET_NGHTTP2_SERVICE_H_INCLEDED
#define NET_NGHTTP2_SERVICE_H_INCLEDED
#include "cpe/utils/utils_types.h"
#include "net_nghttp2_system.h"

NET_BEGIN_DECL

net_nghttp2_service_t net_nghttp2_service_create(net_schedule_t schedule, const char * protocol_name);
void net_nghttp2_service_free(net_nghttp2_service_t service);

net_protocol_t net_nghttp2_service_to_protocol(net_nghttp2_service_t service);

const char * net_nghttp2_service_mine_from_postfix(net_nghttp2_service_t service, const char * postfix);

NET_END_DECL

#endif

