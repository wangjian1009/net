#ifndef NET_WS_SVR_ENDPOINT_H_INCLEDED
#define NET_WS_SVR_ENDPOINT_H_INCLEDED
#include "net_ws_types.h"

NET_BEGIN_DECL

enum net_ws_svr_endpoint_state {
    net_ws_svr_endpoint_state_handshake,
    net_ws_svr_endpoint_state_streaming,
};

typedef void (*net_ws_svr_endpoint_on_msg_text_fun_t)(void * ctx, net_ws_svr_endpoint_t endpoin, const char * msg);
typedef void (*net_ws_svr_endpoint_on_msg_bin_fun_t)(void * ctx, net_ws_svr_endpoint_t endpoin, const void * msg, uint32_t msg_len);

net_ws_svr_endpoint_t net_ws_svr_endpoint_cast(net_endpoint_t endpoint);
net_endpoint_t net_ws_svr_endpoint_stream(net_endpoint_t endpoint);

const char * net_ws_svr_endpoint_path(net_ws_svr_endpoint_t endpoint);
net_address_t net_ws_svr_endpoint_host(net_ws_svr_endpoint_t endpoint);
uint8_t net_ws_svr_endpoint_version(net_ws_svr_endpoint_t endpoint);

net_ws_svr_endpoint_state_t net_ws_svr_endpoint_state(net_ws_svr_endpoint_t endpoint);

void net_ws_svr_endpoint_set_msg_receiver_text(
    net_ws_svr_endpoint_t endpoint,
    void * ctx, net_ws_svr_endpoint_on_msg_text_fun_t fun, void (*ctx_free)(void*));

void net_ws_svr_endpoint_set_msg_receiver_bin(
    net_ws_svr_endpoint_t endpoint,
    void * ctx, net_ws_svr_endpoint_on_msg_bin_fun_t fun, void (*ctx_free)(void*));

const char * net_ws_svr_endpoint_state_str(net_ws_svr_endpoint_state_t state);

NET_END_DECL

#endif
