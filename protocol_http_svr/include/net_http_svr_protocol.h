#ifndef NET_HTTP_SVR_SERVICE_H
#define NET_HTTP_SVR_SERVICE_H
#include "cpe/utils/utils_types.h"
#include "net_http_svr_system.h"

net_http_svr_protocol_t net_http_svr_protocol_create(net_schedule_t schedule, const char * protocol_name);
void net_http_svr_protocol_free(net_http_svr_protocol_t service);

net_protocol_t net_http_svr_protocol_to_protocol(net_http_svr_protocol_t service);

net_http_svr_mount_point_t net_http_svr_protocol_root(net_http_svr_protocol_t service);

const char * net_http_svr_protocol_mine_from_postfix(net_http_svr_protocol_t service, const char * postfix);

#endif
