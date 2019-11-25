#ifndef NET_NGHTTP2_REQUEST_HEADER_H
#define NET_NGHTTP2_REQUEST_HEADER_H
#include "cpe/utils/utils_types.h"
#include "net_nghttp2_system.h"

NET_BEGIN_DECL

struct net_nghttp2_request_header_it {
    net_nghttp2_request_header_t (*next)(net_nghttp2_request_header_it_t it);
    char data[64];
};

void net_nghttp2_request_headers(net_nghttp2_request_t request, net_nghttp2_request_header_it_t it);
const char * net_nghttp2_request_header_name(net_nghttp2_request_header_t header);
const char * net_nghttp2_request_header_value(net_nghttp2_request_header_t header);

#define net_nghttp2_request_header_it_next(__it) ((__it)->next(__it))

NET_END_DECL

#endif
