#ifndef NET_TRANS_HTTP_PROTOCOL_I_H_INCLEDED
#define NET_TRANS_HTTP_PROTOCOL_I_H_INCLEDED
#include "net_trans_manage_i.h"

NET_BEGIN_DECL

struct net_trans_http_protocol {
    uint32_t m_ref_count;
};

net_trans_http_protocol_t net_trans_http_protocol_create(net_trans_manage_t manage);
void net_trans_http_protocol_free(net_trans_http_protocol_t trans_protocol);

NET_END_DECL

#endif
