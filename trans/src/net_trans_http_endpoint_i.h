#ifndef NET_TRANS_HTTP_ENDPOINT_I_H_INCLEDED
#define NET_TRANS_HTTP_ENDPOINT_I_H_INCLEDED
#include "net_http_types.h"
#include "net_trans_manage_i.h"

NET_BEGIN_DECL

struct net_trans_http_endpoint {
    net_trans_manage_t m_manage;
};

net_trans_http_endpoint_t net_trans_ns_cli_endpoint_create(net_trans_manage_t manage);
void net_trans_ns_cli_endpoint_free(net_trans_http_endpoint_t trans_http);

int net_trans_http_endpoint_init(net_http_endpoint_t http_endpoint);
void net_trans_http_endpoint_fini(net_http_endpoint_t http_endpoint);
int net_trans_http_endpoint_on_state_change(net_http_endpoint_t http_endpoint, net_http_state_t from_state);

NET_END_DECL

#endif
