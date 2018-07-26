#ifndef NET_WS_ENDPOINT_I_H_INCLEDED
#define NET_WS_ENDPOINT_I_H_INCLEDED
#include "wslay/wslay.h"
#include "net_ws_endpoint.h"

NET_BEGIN_DECL

struct net_ws_endpoint {
    char * m_url;
};

int net_ws_endpoint_init(net_endpoint_t endpoint);
void net_ws_endpoint_fini(net_endpoint_t endpoint);
int net_ws_endpoint_input(net_endpoint_t endpoint);

NET_END_DECL

#endif
