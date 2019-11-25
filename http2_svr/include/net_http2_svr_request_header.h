#ifndef NET_HTTP2_SVR_REQUEST_HEADER_H
#define NET_HTTP2_SVR_REQUEST_HEADER_H
#include "cpe/utils/utils_types.h"
#include "net_http2_svr_system.h"

NET_BEGIN_DECL

struct net_http2_svr_request_header_it {
    net_http2_svr_request_header_t (*next)(net_http2_svr_request_header_it_t it);
    char data[64];
};

void net_http2_svr_request_headers(net_http2_svr_request_t request, net_http2_svr_request_header_it_t it);
const char * net_http2_svr_request_header_name(net_http2_svr_request_header_t header);
const char * net_http2_svr_request_header_value(net_http2_svr_request_header_t header);

#define net_http2_svr_request_header_it_next(__it) ((__it)->next(__it))

NET_END_DECL

#endif
