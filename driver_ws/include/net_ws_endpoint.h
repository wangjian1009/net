#ifndef NET_WS_CLI_ENDPOINT_H_INCLEDED
#define NET_WS_CLI_ENDPOINT_H_INCLEDED
#include "net_ws_types.h"

NET_BEGIN_DECL

enum net_ws_endpoint_runing_mode {
    net_ws_endpoint_runing_mode_init,
    net_ws_endpoint_runing_mode_cli,
    net_ws_endpoint_runing_mode_svr,
};

enum net_ws_endpoint_state {
    net_ws_endpoint_state_init,
    net_ws_endpoint_state_handshake,
    net_ws_endpoint_state_streaming,
};

typedef void (*net_ws_endpoint_on_msg_text_fun_t)(void * ctx, net_ws_endpoint_t endpoin, const char * msg);
typedef void (*net_ws_endpoint_on_msg_bin_fun_t)(void * ctx, net_ws_endpoint_t endpoin, const void * msg, uint32_t msg_len);

net_ws_endpoint_t net_ws_endpoint_cast(net_endpoint_t endpoint);
net_endpoint_t net_ws_endpoint_stream(net_endpoint_t endpoint);

void net_ws_endpoint_free(net_ws_endpoint_t endpoint);
    
net_ws_endpoint_state_t net_ws_endpoint_state(net_ws_endpoint_t endpoint);
net_ws_endpoint_runing_mode_t net_ws_endpoint_runing_mode(net_ws_endpoint_t endpoint);
int net_ws_endpoint_set_runing_mode(net_ws_endpoint_t endpoint, net_ws_endpoint_runing_mode_t runing_mode);
net_endpoint_t net_ws_endpoint_base_endpoint(net_ws_endpoint_t endpoint);

const char * net_ws_endpoint_path(net_ws_endpoint_t endpoint);
int net_ws_endpoint_set_path(net_ws_endpoint_t endpoint, const char * path);

net_address_t net_ws_endpoint_host(net_ws_endpoint_t endpoint);
int net_ws_endpoint_set_host(net_ws_endpoint_t endpoint, net_address_t host);

int net_ws_endpoint_send_msg_text(net_ws_endpoint_t endpoin, const char * msg);
int net_ws_endpoint_send_msg_bin(net_ws_endpoint_t endpoint, const void * msg, uint32_t msg_len);

void net_ws_endpoint_set_msg_receiver_text(
    net_ws_endpoint_t endpoint,
    void * ctx, net_ws_endpoint_on_msg_text_fun_t fun, void (*ctx_free)(void*));

void net_ws_endpoint_set_msg_receiver_bin(
    net_ws_endpoint_t endpoint,
    void * ctx, net_ws_endpoint_on_msg_bin_fun_t fun, void (*ctx_free)(void*));

const char * net_ws_endpoint_state_str(net_ws_endpoint_state_t state);
const char * net_ws_endpoint_runing_mode_str(net_ws_endpoint_runing_mode_t runing_mode);

NET_END_DECL

#endif
