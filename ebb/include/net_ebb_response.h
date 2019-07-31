#ifndef NET_EBB_RESPONSE_H
#define NET_EBB_RESPONSE_H
#include "net_ebb_system.h"

NET_BEGIN_DECL

net_ebb_response_t net_ebb_response_create(net_ebb_request_t request);
net_ebb_response_t net_ebb_response_get(net_ebb_request_t request);
void net_ebb_response_free(net_ebb_response_t request);

net_ebb_request_transfer_encoding_t net_ebb_response_transfer_encoding(net_ebb_response_t response);

NET_END_DECL

#endif
