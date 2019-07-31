#ifndef NET_EBB_REQUEST_HEADER_H
#define NET_EBB_REQUEST_HEADER_H
#include "cpe/utils/utils_types.h"
#include "net_ebb_system.h"

NET_BEGIN_DECL

struct net_ebb_request_header_it {
    net_ebb_request_header_t (*next)(net_ebb_request_header_it_t it);
    char data[64];
};

void net_ebb_request_headers(net_ebb_request_t request, net_ebb_request_header_it_t it);
const char * net_ebb_request_header_name(net_ebb_request_header_t header);
const char * net_ebb_request_header_value(net_ebb_request_header_t header);

#define net_ebb_request_header_it_next(__it) ((__it)->next(__it))

NET_END_DECL

#endif
